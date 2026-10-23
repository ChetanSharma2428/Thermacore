# Thermacore ESP32 Cloud Firmware Guide

This firmware upgrades the physical Thermacore hardware from a local web server into a cloud-connected IoT edge device using Firebase Realtime Database over secure HTTPS/TLS.

---

## 1. Hardware Pinout & Wiring Reference (Preserved)

No wiring or pin changes are required. The hardware assignments match the original configuration:

| Component | Pin / Bus | Details |
| :--- | :--- | :--- |
| **I2C SDA** | GPIO 21 | Shared I2C Data bus |
| **I2C SCL** | GPIO 22 | Shared I2C Clock bus |
| **LCD 16x2** | I2C `0x27` | LiquidCrystal_I2C display |
| **DHT11** | GPIO 5 | Cabin temperature sensor |
| **DS18B20** | GPIO 4 | Outside probe (OneWire bus with 4.7kΩ pull-up) |
| **INA219** | I2C `0x40` | Battery voltage & current shunt |
| **MPU6050** | I2C `0x68` | 6-DoF accelerometer & gyro |
| **BME280 / BMP280** | I2C `0x76` or `0x77` | Barometric pressure & altitude |
| **L298N ENA** | GPIO 27 | Enable A pin (PWM at 255) |
| **L298N IN1** | GPIO 26 | Peltier direction 1 |
| **L298N IN2** | GPIO 25 | Peltier direction 2 |
| **L298N IN3** | GPIO 18 | Vibration motor direction 1 |
| **L298N IN4** | GPIO 19 | Vibration motor direction 2 |
| **Buzzer** | GPIO 14 | Auditory alert signal |

---

## 2. Required Arduino IDE Libraries

Install these libraries via **Arduino IDE -> Sketch -> Include Library -> Manage Libraries...**:

1. **ArduinoJson** by Benoit Blanchon (Version 6.x or 7.x)
2. **LiquidCrystal I2C** by Frank de Brabander
3. **Adafruit BME280 Library** by Adafruit
4. **Adafruit BMP280 Library** by Adafruit
5. **Adafruit MPU6050** by Adafruit
6. **Adafruit INA219** by Adafruit
7. **Adafruit Unified Sensor** by Adafruit
8. **DHT sensor library** by Adafruit
9. **OneWire** by Paul Stoffregen
10. **DallasTemperature** by Miles Burton

> **Note on Network & SSL**: The firmware uses `WiFiClientSecure.h` and `HTTPClient.h`, which are built into the **ESP32 Arduino Core** (no third-party Firebase wrapper library required). This guarantees clean compilation on any version of the ESP32 board package without dependency conflicts or SSL buffer overflow issues.

---

## 3. Configuration Steps

Open `firmware/thermacore/thermacore.ino` and locate the configuration block:

```cpp
// 1. Wi-Fi credentials
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 2. Firebase Database Host (Without https:// or trailing /)
const char* FIREBASE_HOST = "your-project-id-default-rtdb.firebaseio.com";

// 3. Database Secret / Auth Token (Optional for prototype rules)
const char* FIREBASE_AUTH = ""; 

// 4. Device ID (Must match the Device ID in the React Dashboard)
const char* DEVICE_ID     = "thermacore-esp32-01";
```

---

## 4. Flashing to ESP32

1. Connect the ESP32 to your computer via micro-USB or USB-C.
2. In Arduino IDE:
   - **Tools -> Board -> esp32 -> ESP32 Dev Module**
   - **Tools -> Port -> /dev/cu.usbserial...** (macOS) or **COMx** (Windows)
   - **Tools -> Upload Speed -> 921600** (or 115200)
3. Click the **Upload** arrow button.
4. If the console displays `Connecting........_____.....`, press and hold the **BOOT** button on the ESP32 until the flashing progress bar begins.
5. Once flashing completes (`Hard resetting via RTS pin...`), open **Tools -> Serial Monitor** at **115200 baud**.
6. You will see:
   ```
   [WIFI] Connected! IP: 192.168.x.x
   [SENSOR] BME280 Detected (I2C)
   [FIREBASE] Reported state acknowledged (HTTP 200)
   ```

---

## 5. Non-Blocking Safety Architecture

- **Independent Sensor Loop**: Sensors are polled every 500ms using `millis()`. Network delays never block sensor measurements or buzzer alerts.
- **Fail-Safe Outage Recovery**: If Wi-Fi disconnects, the ESP32 logs the outage and continues monitoring terrain tilt and battery health locally. Reconnection is attempted periodically in the background.
- **Safe Boot Actuator State**: All L298N control pins (`IN1`, `IN2`, `IN3`, `IN4`) boot to `LOW` (OFF) to prevent accidental thermal stress or unwanted motor activation.
