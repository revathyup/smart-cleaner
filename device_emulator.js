const awsIoT = require('aws-iot-device-sdk');

// Enhanced device state with all smart cleaner features
const deviceState = {
  deviceId: 'cleaner-' + Math.floor(Math.random() * 1000),
  battery: 100,
  position: { x: 0, y: 0, heading: 0 },
  status: 'docked', // docked, cleaning, returning, paused, charging, error
  cleaningMode: 'auto', // auto, spot, edge, manual
  isCharging: true,
  
  sensors: {
    dustBin: false, // true = full
    brushes: 'normal', // normal, tangled, missing
    wheels: 'normal', // normal, stuck, damaged
    cliff: false, // cliff detected
    bump: false, // obstacle hit
    dirt: Math.floor(Math.random() * 100) // dirt level sensor
  },
  
  mapping: {
    roomsDiscovered: ['living_room', 'kitchen'],
    currentRoom: 'living_room',
    obstaclesDetected: [],
    cleanedAreas: 0, // square meters
    totalArea: 120 // total discoverable area
  },
  
  maintenance: {
    filterLife: 85, // percentage
    brushHours: 120, // hours remaining
    lastCleaned: Date.now() - (24 * 60 * 60 * 1000), // yesterday
    totalCleaningTime: 0 // minutes
  },
  
  schedule: {
    enabled: false,
    time: '09:00',
    days: ['monday', 'wednesday', 'friday'],
    nextClean: null
  },
  
  stats: {
    totalCleans: 45,
    totalRuntime: 2850, // minutes
    averageCleanTime: 63 // minutes
  }
};

// AWS IoT Device connection
const device = awsIoT.device({
  keyPath: './certs/private.key',
  certPath: './certs/certificate.pem',
  caPath: './certs/RootCA1.pem',
  clientId: 'cleaner-' + Math.floor(Math.random() * 1000), // Simulate multiple devices
  host: 'a38j1yxtgtu8ch-ats.iot.eu-north-1.amazonaws.com' // Find in AWS IoT Core Settings
});

// 2. Embedded device state
const deviceState = {
  battery: 100,
  position: { x: 0, y: 0 },
  status: 'idle'
};

// 3. Simulate sensor readings (like real hardware would)
function simulateSensors() {
  const now = Date.now();
  
  // Battery simulation
  if (deviceState.isCharging && deviceState.battery < 100) {
    deviceState.battery = Math.min(100, deviceState.battery + 2);
  } else if (deviceState.status === 'cleaning' && deviceState.battery > 0) {
    deviceState.battery = Math.max(0, deviceState.battery - 0.8);
  }
  
  // Position and movement simulation
  if (deviceState.status === 'cleaning' || deviceState.status === 'returning') {
    deviceState.position.x += Math.random() * 2 - 1; // Random walk
    deviceState.position.y += Math.random() * 2 - 1;
    deviceState.position.heading = (deviceState.position.heading + (Math.random() * 30 - 15)) % 360;
    
    // Keep within bounds
    deviceState.position.x = Math.max(-50, Math.min(50, deviceState.position.x));
    deviceState.position.y = Math.max(-50, Math.min(50, deviceState.position.y));
  }
  
  // Cleaning progress simulation
  if (deviceState.status === 'cleaning') {
    deviceState.mapping.cleanedAreas += 0.1; // 0.1 m² per cycle
    deviceState.maintenance.totalCleaningTime += 0.033; // 2 seconds per cycle = 0.033 minutes
    
    // Random sensor events
    if (Math.random() < 0.05) { // 5% chance
      deviceState.sensors.bump = true;
      setTimeout(() => { deviceState.sensors.bump = false; }, 3000);
    }
    
    if (Math.random() < 0.02) { // 2% chance
      deviceState.sensors.cliff = true;
      setTimeout(() => { deviceState.sensors.cliff = false; }, 2000);
    }
  }
  
  // Maintenance degradation
  if (deviceState.status === 'cleaning') {
    deviceState.maintenance.filterLife = Math.max(0, deviceState.maintenance.filterLife - 0.01);
    deviceState.maintenance.brushHours = Math.max(0, deviceState.maintenance.brushHours - 0.033);
  }
  
  // Auto-return when battery low
  if (deviceState.battery <= 15 && deviceState.status === 'cleaning') {
    deviceState.status = 'returning';
    publishAlert('Low battery - returning to dock');
  }
  
  // Auto-dock when returning
  if (deviceState.status === 'returning' && 
      Math.abs(deviceState.position.x) < 2 && 
      Math.abs(deviceState.position.y) < 2) {
    deviceState.status = 'docked';
    deviceState.isCharging = true;
    deviceState.position = { x: 0, y: 0, heading: 0 };
    publishAlert('Successfully docked and charging');
  }
  
  // Dust bin full simulation
  if (deviceState.mapping.cleanedAreas > 50 && !deviceState.sensors.dustBin) {
    deviceState.sensors.dustBin = true;
    publishAlert('Dust bin is full - please empty');
  }
}

// Publishing functions
function publishTelemetry() {
  const telemetry = {
    timestamp: Date.now(),
    deviceId: deviceState.deviceId,
    ...deviceState
  };
  
  device.publish('cleaner/telemetry', JSON.stringify(telemetry));
}

function publishDeviceInfo() {
  const deviceInfo = {
    deviceId: deviceState.deviceId,
    model: 'SmartCleaner Pro X1',
    firmware: '2.1.4',
    registered: Date.now(),
    capabilities: ['mapping', 'scheduling', 'voice_control', 'app_control']
  };
  
  device.publish('cleaner/device/register', JSON.stringify(deviceInfo));
}

function publishStatusChange(oldStatus, newStatus) {
  const statusChange = {
    timestamp: Date.now(),
    deviceId: deviceState.deviceId,
    oldStatus,
    newStatus,
    battery: deviceState.battery,
    position: deviceState.position
  };
  
  device.publish('cleaner/status', JSON.stringify(statusChange));
}

function publishAlert(message) {
  const alert = {
    timestamp: Date.now(),
    deviceId: deviceState.deviceId,
    level: 'info',
    message,
    position: deviceState.position
  };
  
  device.publish('cleaner/alerts', JSON.stringify(alert));
}

function publishError(message) {
  deviceState.status = 'error';
  const error = {
    timestamp: Date.now(),
    deviceId: deviceState.deviceId,
    level: 'error',
    message,
    deviceState: { ...deviceState }
  };
  
  device.publish('cleaner/errors', JSON.stringify(error));
}

function publishScheduleUpdate() {
  device.publish('cleaner/schedule/status', JSON.stringify({
    timestamp: Date.now(),
    deviceId: deviceState.deviceId,
    schedule: deviceState.schedule
  }));
}

function publishMaintenanceUpdate() {
  device.publish('cleaner/maintenance/status', JSON.stringify({
    timestamp: Date.now(),
    deviceId: deviceState.deviceId,
    maintenance: deviceState.maintenance
  }));
}

// Schedule checker
function checkSchedule() {
  if (!deviceState.schedule.enabled) return;
  
  const now = new Date();
  const currentDay = now.toLocaleDateString('en-US', { weekday: 'long' }).toLowerCase();
  const currentTime = now.toTimeString().substr(0, 5);
  
  if (deviceState.schedule.days.includes(currentDay) && 
      currentTime === deviceState.schedule.time &&
      deviceState.status === 'docked' &&
      deviceState.battery > 50) {
    
    console.log('📅 Scheduled cleaning started');
    handleControlCommand({ action: 'start_cleaning', mode: 'auto' });
  }
}

// Maintenance alerts
function checkMaintenance() {
  const alerts = [];
  
  if (deviceState.maintenance.filterLife < 10) {
    alerts.push('Filter needs replacement');
  }
  
  if (deviceState.maintenance.brushHours < 10) {
    alerts.push('Brushes need maintenance');
  }
  
  const daysSinceLastCleaned = (Date.now() - deviceState.maintenance.lastCleaned) / (1000 * 60 * 60 * 24);
  if (daysSinceLastCleaned > 30) {
    alerts.push('Device needs cleaning and maintenance');
  }
  
  if (deviceState.sensors.dustBin) {
    alerts.push('Dust bin is full');
  }
  
  alerts.forEach(alert => publishAlert(alert));
}

// Main simulation loop
console.log(`🚀 Starting Smart Cleaner Simulator - Device ID: ${deviceState.deviceId}`);

setInterval(() => {
  simulateSensors();
  publishTelemetry();
}, 2000); // Match your hardware's telemetry interval
