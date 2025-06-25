// smart_cleaner.ino - Main Arduino/ESP32 code
// This is the core embedded logic you'd actually program onto hardware

#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>  // For MQTT/AWS IoT

// ==================== HARDWARE PINS ====================
#define MOTOR_LEFT_PWM    25
#define MOTOR_LEFT_DIR    26
#define MOTOR_RIGHT_PWM   27
#define MOTOR_RIGHT_DIR   14
#define ULTRASONIC_TRIG   5
#define ULTRASONIC_ECHO   18
#define BATTERY_PIN       34
#define STATUS_LED        2

// ==================== CONFIGURATION ====================
const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";
const char* AWS_IOT_ENDPOINT = "your-endpoint.iot.region.amazonaws.com";
const char* DEVICE_ID = "smart-cleaner-001";

// ==================== GLOBAL VARIABLES ====================
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

struct CleanerState {
    int x, y;                    // Position (simulated)
    int battery_level;           // 0-100%
    bool is_cleaning;            // Active cleaning mode
    String current_mode;         // "random", "systematic", "spot"
    unsigned long start_time;    // When cleaning started
    int dirt_collected;          // Count of dirt spots cleaned
    bool obstacle_detected;      // Current obstacle status
};

CleanerState cleaner = {50, 50, 100, false, "idle", 0, 0, false};

// ==================== CORE FUNCTIONS ====================

void setup() {
    Serial.begin(115200);
    
    // Initialize hardware pins
    pinMode(MOTOR_LEFT_PWM, OUTPUT);
    pinMode(MOTOR_LEFT_DIR, OUTPUT);
    pinMode(MOTOR_RIGHT_PWM, OUTPUT);
    pinMode(MOTOR_RIGHT_DIR, OUTPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    pinMode(STATUS_LED, OUTPUT);
    
    // Connect to WiFi
    connectWiFi();
    
    // Setup MQTT for AWS IoT
    mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);
    mqttClient.setCallback(onMqttMessage);
    
    Serial.println("Smart Cleaner initialized!");
    blinkLED(3); // Startup indication
}

void loop() {
    // Main control loop - runs continuously
    
    // 1. Check WiFi/MQTT connection
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();
    
    // 2. Read sensors
    readSensors();
    
    // 3. Make cleaning decisions
    if (cleaner.is_cleaning) {
        executeCleaningLogic();
    }
    
    // 4. Send telemetry to AWS
    static unsigned long lastTelemetry = 0;
    if (millis() - lastTelemetry > 5000) { // Every 5 seconds
        sendTelemetry();
        lastTelemetry = millis();
    }
    
    // 5. Update status LED
    updateStatusLED();
    
    delay(100); // Small delay for stability
}

// ==================== SENSOR FUNCTIONS ====================

void readSensors() {
    // Read battery level (voltage divider)
    int batteryRaw = analogRead(BATTERY_PIN);
    cleaner.battery_level = map(batteryRaw, 0, 4095, 0, 100);
    
    // Check for obstacles using ultrasonic sensor
    cleaner.obstacle_detected = checkObstacle();
}

bool checkObstacle() {
    // Send ultrasonic pulse
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    // Read echo
    long duration = pulseIn(ULTRASONIC_ECHO, HIGH);
    int distance = duration * 0.034 / 2; // Convert to cm
    
    return (distance < 20); // Obstacle if closer than 20cm
}

// ==================== MOVEMENT FUNCTIONS ====================

void executeCleaningLogic() {
    // Check battery level
    if (cleaner.battery_level < 15) {
        Serial.println("Low battery - returning home");
        returnToBase();
        return;
    }
    
    // Execute movement based on current mode
    if (cleaner.current_mode == "random") {
        randomMovement();
    } else if (cleaner.current_mode == "systematic") {
        systematicMovement();
    } else if (cleaner.current_mode == "spot") {
        spotCleaning();
    }
}

void randomMovement() {
    if (cleaner.obstacle_detected) {
        // Turn away from obstacle
        turnRandom();
        delay(500);
    } else {
        // Move forward
        moveForward();
        
        // Randomly change direction occasionally
        if (random(100) < 5) { // 5% chance each loop
            turnRandom();
            delay(300);
        }
    }
}

void systematicMovement() {
    // Simple back-and-forth pattern
    static bool movingRight = true;
    static int currentRow = 0;
    
    if (cleaner.obstacle_detected) {
        if (movingRight) {
            // Hit right wall, move down and turn left
            moveBackward();
            delay(200);
            turnLeft();
            moveForward();
            delay(500);
            turnLeft();
            movingRight = false;
            currentRow++;
        } else {
            // Hit left wall, move down and turn right
            moveBackward();
            delay(200);
            turnRight();
            moveForward();
            delay(500);
            turnRight();
            movingRight = true;
            currentRow++;
        }
    } else {
        moveForward();
    }
}

void spotCleaning() {
    // Intensive cleaning in small area
    static int spotPhase = 0;
    static unsigned long phaseStartTime = millis();
    
    if (millis() - phaseStartTime > 2000) { // Change pattern every 2 seconds
        spotPhase = (spotPhase + 1) % 4;
        phaseStartTime = millis();
    }
    
    switch (spotPhase) {
        case 0: moveForward(); break;
        case 1: turnRight(); break;
        case 2: moveBackward(); break;
        case 3: turnLeft(); break;
    }
}

// ==================== MOTOR CONTROL ====================

void moveForward() {
    digitalWrite(MOTOR_LEFT_DIR, HIGH);
    digitalWrite(MOTOR_RIGHT_DIR, HIGH);
    analogWrite(MOTOR_LEFT_PWM, 200);  // Adjust speed as needed
    analogWrite(MOTOR_RIGHT_PWM, 200);
    
    // Simulate position update
    cleaner.y += 1;
}

void moveBackward() {
    digitalWrite(MOTOR_LEFT_DIR, LOW);
    digitalWrite(MOTOR_RIGHT_DIR, LOW);
    analogWrite(MOTOR_LEFT_PWM, 200);
    analogWrite(MOTOR_RIGHT_PWM, 200);
    
    cleaner.y -= 1;
}

void turnLeft() {
    digitalWrite(MOTOR_LEFT_DIR, LOW);
    digitalWrite(MOTOR_RIGHT_DIR, HIGH);
    analogWrite(MOTOR_LEFT_PWM, 150);
    analogWrite(MOTOR_RIGHT_PWM, 150);
}

void turnRight() {
    digitalWrite(MOTOR_LEFT_DIR, HIGH);
    digitalWrite(MOTOR_RIGHT_DIR, LOW);
    analogWrite(MOTOR_LEFT_PWM, 150);
    analogWrite(MOTOR_RIGHT_PWM, 150);
}

void turnRandom() {
    if (random(2)) {
        turnLeft();
    } else {
        turnRight();
    }
}

void stopMotors() {
    analogWrite(MOTOR_LEFT_PWM, 0);
    analogWrite(MOTOR_RIGHT_PWM, 0);
}

// ==================== AWS IOT FUNCTIONS ====================

void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
}

void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to AWS IoT...");
        
        if (mqttClient.connect(DEVICE_ID)) {
            Serial.println("connected!");
            
            // Subscribe to control topics
            mqttClient.subscribe("cleaner/control/start");
            mqttClient.subscribe("cleaner/control/stop");
            mqttClient.subscribe("cleaner/control/mode");
            mqttClient.subscribe("cleaner/control/home");
            
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.println("Received: " + String(topic) + " -> " + message);
    
    // Handle different control commands
    if (String(topic) == "cleaner/control/start") {
        startCleaning(message); // message contains mode
    } else if (String(topic) == "cleaner/control/stop") {
        stopCleaning();
    } else if (String(topic) == "cleaner/control/mode") {
        changeMode(message);
    } else if (String(topic) == "cleaner/control/home") {
        returnToBase();
    }
}

void sendTelemetry() {
    // Create JSON payload
    StaticJsonDocument<300> doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = millis();
    doc["battery"] = cleaner.battery_level;
    doc["position"]["x"] = cleaner.x;
    doc["position"]["y"] = cleaner.y;
    doc["is_cleaning"] = cleaner.is_cleaning;
    doc["mode"] = cleaner.current_mode;
    doc["dirt_collected"] = cleaner.dirt_collected;
    doc["obstacle_detected"] = cleaner.obstacle_detected;
    
    if (cleaner.is_cleaning) {
        doc["runtime"] = (millis() - cleaner.start_time) / 1000;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publish to AWS IoT
    mqttClient.publish("cleaner/telemetry", jsonString.c_str());
    
    Serial.println("Telemetry sent: " + jsonString);
}

// ==================== CONTROL FUNCTIONS ====================

void startCleaning(String mode) {
    if (!cleaner.is_cleaning) {
        cleaner.is_cleaning = true;
        cleaner.current_mode = mode.length() > 0 ? mode : "random";
        cleaner.start_time = millis();
        
        Serial.println("Started cleaning in " + cleaner.current_mode + " mode");
        
        // Send confirmation to AWS
        StaticJsonDocument<100> doc;
        doc["event"] = "cleaning_started";
        doc["mode"] = cleaner.current_mode;
        String response;
        serializeJson(doc, response);
        mqttClient.publish("cleaner/status", response.c_str());
    }
}

void stopCleaning() {
    if (cleaner.is_cleaning) {
        cleaner.is_cleaning = false;
        stopMotors();
        
        Serial.println("Cleaning stopped");
        
        // Send confirmation to AWS
        StaticJsonDocument<100> doc;
        doc["event"] = "cleaning_stopped";
        doc["total_runtime"] = (millis() - cleaner.start_time) / 1000;
        String response;
        serializeJson(doc, response);
        mqttClient.publish("cleaner/status", response.c_str());
    }
}

void changeMode(String newMode) {
    cleaner.current_mode = newMode;
    Serial.println("Mode changed to: " + newMode);
}

void returnToBase() {
    stopCleaning();
    
    // Simple return-to-base logic (in real implementation, would use navigation)
    Serial.println("Returning to base...");
    
    // Simulate returning to charging dock
    cleaner.x = 50;
    cleaner.y = 50;
    
    // Send status update
    StaticJsonDocument<100> doc;
    doc["event"] = "returned_to_base";
    doc["battery"] = cleaner.battery_level;
    String response;
    serializeJson(doc, response);
    mqttClient.publish("cleaner/status", response.c_str());
}

// ==================== UTILITY FUNCTIONS ====================

void updateStatusLED() {
    if (cleaner.is_cleaning) {
        // Blink while cleaning
        digitalWrite(STATUS_LED, (millis() / 500) % 2);
    } else if (cleaner.battery_level < 20) {
        // Fast blink for low battery
        digitalWrite(STATUS_LED, (millis() / 200) % 2);
    } else {
        // Solid on when idle and charged
        digitalWrite(STATUS_LED, HIGH);
    }
}

void blinkLED(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(STATUS_LED, HIGH);
        delay(200);
        digitalWrite(STATUS_LED, LOW);
        delay(200);
    }
}