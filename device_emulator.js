const awsIoT = require('aws-iot-device-sdk');

// 1. Load certificates (from AWS IoT Core)
const device = awsIoT.device({
  keyPath: './certs/private.key',
  certPath: './certs/certificate.pem',
  caPath: './certs/RootCA1.pem',
  clientId: 'cleaner-' + Math.floor(Math.random() * 1000), // Simulate multiple devices
  host: 'YOUR_ENDPOINT.iot.REGION.amazonaws.com' // Find in AWS IoT Core Settings
});

// 2. Embedded device state
const deviceState = {
  battery: 100,
  position: { x: 0, y: 0 },
  status: 'idle'
};

// 3. Simulate sensor readings (like real hardware would)
function simulateSensors() {
  deviceState.battery -= 0.5;
  deviceState.position.x += 1;
  deviceState.position.y += Math.sin(Date.now() / 1000);
}

// 4. Publish telemetry (like a real MCU would)
function publishTelemetry() {
  device.publish('cleaner/telemetry', JSON.stringify({
    timestamp: Date.now(),
    ...deviceState
  }));
}

// 5. Handle cloud commands (like OTA updates)
device.on('message', (topic, payload) => {
  if (topic === 'cleaner/control') {
    const command = JSON.parse(payload.toString());
    console.log('Received command:', command);
    // Implement command handling (e.g., change cleaning mode)
  }
});

// Main loop (replaces hardware's while(1))
setInterval(() => {
  simulateSensors();
  publishTelemetry();
}, 2000); // Match your hardware's telemetry interval