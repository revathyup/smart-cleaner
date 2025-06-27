# Smart Cleaner IoT Device Emulator

This project simulates a smart robotic cleaner as an IoT device, connecting to AWS IoT Core using MQTT. It publishes telemetry, handles remote commands, simulates sensors and maintenance, and supports scheduling and alerts—just like a real smart cleaner!

---

## Features

- **AWS IoT Core MQTT connection** (X.509 certificate-based)
- **Simulated device state:** battery, position, cleaning modes, sensors, mapping, maintenance, schedule, and stats
- **Publishes telemetry** to `cleaner/telemetry` every 2 seconds
- **Receives remote commands** via `cleaner/control` (start, pause, dock, etc.)
- **Publishes alerts, status changes, errors, and maintenance info**
- **Simulates sensor events** (bump, cliff, dust bin full, etc.)
- **Auto-docking and maintenance checks**
- **Easily extensible for more features or multiple devices**

---

## Folder Structure

```
smart-cleaner/
├── certs/                  # AWS IoT certificates (not in repo)
├── device_emulator.js      # Main simulator code
├── package.json
└── README.md
```

---

## Setup

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/smart-cleaner.git
cd smart-cleaner
```

### 2. Install Dependencies

```bash
npm install aws-iot-device-sdk
```

### 3. AWS IoT Setup

- Register a Thing in AWS IoT Core.
- Create and download device certificates and keys.
- Download the Amazon Root CA.
- Place these files in the `certs/` folder:
  - `certificate.pem`
  - `private.key`
  - `RootCA1.pem`

### 4. Update Device Endpoint

In `device_emulator.js`, set your AWS IoT endpoint:
```js
host: 'YOUR_ENDPOINT_HERE' // e.g. a38j1yxtgtu8ch-ats.iot.eu-north-1.amazonaws.com
```

---

## Usage

### Start the Emulator

```bash
node device_emulator.js
```

You should see logs for telemetry and status.

---

### Interact via AWS IoT Core

#### **View Telemetry**
- Go to AWS IoT Core → Test → MQTT test client.
- Subscribe to:  
  ```
  cleaner/telemetry
  ```

#### **Send Commands**
- Publish to:  
  ```
  cleaner/control
  ```
- Example payloads:
  - Start cleaning:
    ```json
    { "action": "start_cleaning", "mode": "auto" }
    ```
  - Pause:
    ```json
    { "action": "pause" }
    ```
  - Dock:
    ```json
    { "action": "dock" }
    ```

#### **View Alerts and Status**
- Subscribe to:
  - `cleaner/alerts`
  - `cleaner/status`
  - `cleaner/errors`
  - `cleaner/device/register`

---

## Customization

- **Add more commands** in `handleControlCommand`.
- **Simulate more sensors** in `simulateSensors`.
- **Enable scheduling** by setting `deviceState.schedule.enabled = true`.
- **Run multiple devices** by starting multiple instances with different `deviceId`s.

---

## Security

- **Never commit your certificates or private keys to version control!**
- Add `certs/` to your `.gitignore`.

---

## License

MIT