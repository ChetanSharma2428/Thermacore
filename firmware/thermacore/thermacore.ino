/*
 * ==============================================================================
 *  Project: Thermacore — Autonomous Thermal & Environmental Telemetry
 *  Firmware: Cloud Edition (Firebase Realtime Database over HTTPS)
 *  Target Board: ESP32 Dev Module / NodeMCU-32S
 * ==============================================================================
 *
 *  HARDWARE PIN CONNECTIONS (PRESERVED):
 *  - I2C Bus:          SDA = GPIO 21, SCL = GPIO 22
 *  - LCD (I2C):        Address 0x27 (16x2 Display)
 *  - DHT11 Sensor:     Data = GPIO 5
 *  - DS18B20 Sensor:   OneWire Bus = GPIO 4 (4.7k pull-up resistor to 3.3V)
 *  - MPU6050 (I2C):    Address 0x68
 *  - INA219 (I2C):     Address 0x40
 *  - BME280/BMP280:    Address 0x76 or 0x77
 *  - L298N Motor Driver:
 *      ENA = GPIO 27 (PWM enable driven at 255)
 *      IN1 = GPIO 26 (Peltier Direction 1)
 *      IN2 = GPIO 25 (Peltier Direction 2)
 *      IN3 = GPIO 18 (Vibration Motor Direction 1)
 *      IN4 = GPIO 19 (Vibration Motor Direction 2)
 *  - Buzzer:           GPIO 14
 * ==============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_INA219.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ==============================================================================
// 1. NETWORK & FIREBASE CONFIGURATION
// Loads credentials securely from "secrets.h" (ignored by git).
// If secrets.h does not exist, copy secrets.h.example to secrets.h
// ==============================================================================
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #define SECRET_WIFI_SSID     "YOUR_WIFI_SSID"
  #define SECRET_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
  #define SECRET_FIREBASE_HOST "your-project-id-default-rtdb.asia-southeast1.firebasedatabase.app"
  #define SECRET_FIREBASE_AUTH ""
  #define SECRET_DEVICE_ID     "thermacore-esp32-01"
#endif

// Applied Configuration
const char* WIFI_SSID     = SECRET_WIFI_SSID;
const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
const char* FIREBASE_HOST = SECRET_FIREBASE_HOST;
const char* FIREBASE_AUTH = SECRET_FIREBASE_AUTH;
const char* DEVICE_ID     = SECRET_DEVICE_ID;

// Firmware Version
const char* FW_VERSION    = "2.0.0-cloud";

// ==============================================================================
// 2. HARDWARE PIN DEFINITIONS
// ==============================================================================
#define DHTPIN 5
#define DHTTYPE DHT11
#define ONE_WIRE_BUS 4

#define ENA 27
#define IN1 26 // Peltier Direction 1
#define IN2 25 // Peltier Direction 2
#define IN3 18 // Vibration Motor Direction 1
#define IN4 19 // Vibration Motor Direction 2
#define BUZZER 14

// ==============================================================================
// 3. HARDWARE DRIVER INSTANCES
// ==============================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;
Adafruit_INA219 ina219;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

WiFiClientSecure secureClient;
HTTPClient https;

// ==============================================================================
// 4. TIMERS & STATE TRACKING
// ==============================================================================
unsigned long lastSensorReadTime  = 0;
unsigned long lastTelemetryUpload = 0;
unsigned long lastCommandCheck    = 0;
unsigned long lastHeartbeatTime   = 0;
unsigned long alertClearTime      = 0;
unsigned long lastWifiRetryTime   = 0;

const unsigned long SENSOR_INTERVAL    = 500;   // Read sensors every 500ms
const unsigned long TELEMETRY_INTERVAL = 1500;  // Push to cloud every 1.5s
const unsigned long COMMAND_INTERVAL   = 1000;  // Poll for commands every 1.0s
const unsigned long HEARTBEAT_INTERVAL = 10000; // Heartbeat status every 10s

int activePressureSensor = 0; // 0=None, 1=BME280, 2=BMP280
int peltierState         = 0; // 0=Off, 1=Heating, 2=Cooling
int vibrationState       = 0; // 0=Off, 1=On
bool roughTerrainAlert   = false;
String lastLcdMsg        = "";
String lastExecutedCmdId = "";

// Global Telemetry Variables
float g_alt = 0.0, g_press = 0.0;
float g_accX = 0.0, g_accY = -9.5, g_accZ = 0.0;
float g_volts = 0.0, g_mA = 0.0;
float g_cabinTemp = 0.0, g_outTemp = 0.0;

// ==============================================================================
// 5. HELPER FUNCTIONS
// ==============================================================================

// LCD Message Helper (non-blocking alert clear)
void displayMessage(String msgLine1, String msgLine2, int durationMs = 0) {
  String combined = msgLine1 + msgLine2;
  if (lastLcdMsg != combined) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msgLine1);
    lcd.setCursor(0, 1);
    lcd.print(msgLine2);
    lastLcdMsg = combined;
  }
  if (durationMs > 0) {
    alertClearTime = millis() + durationMs;
  }
}

// Build Firebase REST URL
String buildFirebaseUrl(String path) {
  String url = "https://" + String(FIREBASE_HOST) + "/" + path + ".json";
  if (strlen(FIREBASE_AUTH) > 0) {
    url += "?auth=" + String(FIREBASE_AUTH);
  }
  return url;
}

// Actuator Hardware Control Functions
void applyPeltier(int mode) {
  peltierState = mode;
  if (peltierState == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    displayMessage("Peltier: OFF", "Cloud Confirmed", 2500);
    Serial.println("[ACTUATOR] Peltier -> OFF");
  } else if (peltierState == 1) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH); // Heat
    displayMessage("HEATING MODE", "Cloud Confirmed", 2500);
    Serial.println("[ACTUATOR] Peltier -> HEATING");
  } else if (peltierState == 2) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW); // Cool
    displayMessage("COOLING MODE", "Cloud Confirmed", 2500);
    Serial.println("[ACTUATOR] Peltier -> COOLING");
  }
}

void applyVibration(int state) {
  vibrationState = state;
  if (vibrationState == 1) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    displayMessage("VIBRATION: ON", "De-Icing Active", 2500);
    Serial.println("[ACTUATOR] Vibration -> ON");
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    displayMessage("VIBRATION: OFF", "De-Icing Stopped", 2500);
    Serial.println("[ACTUATOR] Vibration -> OFF");
  }
}

// ==============================================================================
// 6. FIREBASE CLOUD COMMUNICATION (HTTPS REST)
// ==============================================================================

// Upload latest telemetry snapshot to devices/{deviceId}/telemetry
void uploadTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;

  StaticJsonDocument<384> doc;
  doc["alt"]          = serialized(String(g_alt, 1));
  doc["pres"]         = serialized(String(g_press, 1));
  doc["cab"]          = serialized(String(g_cabinTemp, 1));
  doc["out"]          = serialized(String(g_outTemp, 1));
  doc["volts"]        = serialized(String(g_volts, 2));
  doc["mA"]           = serialized(String(g_mA, 1));
  doc["accX"]         = serialized(String(g_accX, 2));
  doc["accY"]         = serialized(String(g_accY, 2));
  doc["accZ"]         = serialized(String(g_accZ, 2));
  doc["peltier"]      = peltierState;
  doc["vib"]          = vibrationState;
  doc["alertTerrain"] = roughTerrainAlert ? 1 : 0;
  doc["alertBat"]     = (g_volts < 3.5 && g_volts > 1.0) ? 1 : 0;
  doc["timestamp"]    = millis();

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  String url = buildFirebaseUrl("devices/" + String(DEVICE_ID) + "/telemetry");

  https.begin(secureClient, url);
  https.addHeader("Content-Type", "application/json");
  int httpCode = https.PUT(jsonPayload);

  if (httpCode == HTTP_CODE_OK || httpCode == 204) {
    // Successfully written
  } else if (httpCode > 0) {
    Serial.printf("[FIREBASE] Telemetry upload warning: HTTP %d\n", httpCode);
  } else {
    Serial.printf("[FIREBASE] Telemetry upload failed: %s\n", https.errorToString(httpCode).c_str());
  }
  https.end();
}

// Report authoritative actuator state back to devices/{deviceId}/reportedState
void reportActuatorState(String commandId) {
  if (WiFi.status() != WL_CONNECTED) return;

  StaticJsonDocument<256> doc;
  doc["peltier"]       = peltierState;
  doc["vibration"]     = vibrationState;
  doc["lastCommandId"] = commandId;
  doc["updatedAt"]     = millis();

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  String url = buildFirebaseUrl("devices/" + String(DEVICE_ID) + "/reportedState");

  https.begin(secureClient, url);
  https.addHeader("Content-Type", "application/json");
  int httpCode = https.PUT(jsonPayload);
  if (httpCode > 0) {
    Serial.printf("[FIREBASE] Reported state acknowledged (HTTP %d)\n", httpCode);
  }
  https.end();
}

// Heartbeat & status update to devices/{deviceId}/status
void updateDeviceStatus(bool isOnline) {
  if (WiFi.status() != WL_CONNECTED) return;

  StaticJsonDocument<256> doc;
  doc["online"]          = isOnline;
  doc["ip"]              = WiFi.localIP().toString();
  doc["lastSeen"]        = millis();
  doc["firmwareVersion"] = FW_VERSION;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  String url = buildFirebaseUrl("devices/" + String(DEVICE_ID) + "/status");

  https.begin(secureClient, url);
  https.addHeader("Content-Type", "application/json");
  https.PUT(jsonPayload);
  https.end();
}

// Fetch and execute pending commands from devices/{deviceId}/commands
void checkCloudCommands() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = buildFirebaseUrl("devices/" + String(DEVICE_ID) + "/commands");

  https.begin(secureClient, url);
  int httpCode = https.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = https.getString();
    if (payload != "null" && payload.length() > 2) {
      StaticJsonDocument<384> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        const char* cmdId     = doc["commandId"];
        const char* actuator  = doc["actuator"];
        const char* status    = doc["status"];
        int requestedState    = doc["requestedState"] | -1;

        // Verify if command is new and in PENDING status
        if (cmdId && String(cmdId) != lastExecutedCmdId && String(status) == "PENDING") {
          Serial.printf("[COMMAND] Processing Command ID: %s | Actuator: %s | State: %d\n",
                        cmdId, actuator, requestedState);

          bool commandApplied = false;

          // 1. Peltier Command (0=OFF, 1=HEAT, 2=COOL)
          if (String(actuator) == "peltier" && requestedState >= 0 && requestedState <= 2) {
            applyPeltier(requestedState);
            commandApplied = true;
          }
          // 2. Vibration Command (0=OFF, 1=ON)
          else if (String(actuator) == "vibration" && (requestedState == 0 || requestedState == 1)) {
            applyVibration(requestedState);
            commandApplied = true;
          } else {
            Serial.printf("[COMMAND] Invalid command parameters rejected!\n");
          }

          if (commandApplied) {
            lastExecutedCmdId = String(cmdId);
            // Confirm back to Firebase reportedState
            reportActuatorState(lastExecutedCmdId);
          }
        }
      }
    }
  }
  https.end();
}

// Non-blocking Wi-Fi reconnection handler
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastWifiRetryTime >= 10000) { // Retry every 10 seconds
      lastWifiRetryTime = now;
      Serial.println("[WIFI] Reconnecting to Wi-Fi...");
      WiFi.reconnect();
    }
  }
}

// ==============================================================================
// 7. ARDUINO SETUP
// ==============================================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Initialize Pin Modes
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Safe Default Output States
  analogWrite(ENA, 255);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(BUZZER, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("THERMACORE CLOUD");
  lcd.setCursor(0, 1);
  lcd.print("CONNECTING WIFI.");

  // Wi-Fi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) {
    delay(400);
    Serial.print(".");
    attempts++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WIFI CONNECTED");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    Serial.println("\n[WIFI] Connected! IP: " + WiFi.localIP().toString());
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WIFI OFFLINE");
    lcd.setCursor(0, 1);
    lcd.print("STANDALONE MODE");
    Serial.println("\n[WIFI] Offline. Operating in local safety fallback mode.");
  }

  // Initialize Sensors
  dht.begin();
  ds18b20.begin();
  mpu.begin();
  ina219.begin();

  if (bme.begin(0x76) || bme.begin(0x77)) {
    activePressureSensor = 1;
    Serial.println("[SENSOR] BME280 Detected (I2C)");
  } else if (bmp.begin(0x76) || bmp.begin(0x77)) {
    activePressureSensor = 2;
    Serial.println("[SENSOR] BMP280 Detected (I2C)");
  } else {
    Serial.println("[SENSOR] No Barometer detected (Altitude/Pressure offline)");
  }

  // Configure Secure Client for HTTPS
  // In development/prototype, insecure mode allows self-signed/Firebase root cert without bundling heavy CA bundle
  secureClient.setInsecure();
  secureClient.setTimeout(4); // 4 second timeout

  // Initial cloud handshake
  if (WiFi.status() == WL_CONNECTED) {
    updateDeviceStatus(true);
    reportActuatorState("INIT_BOOT");
  }

  delay(1500);
  displayMessage("Situation Normal", "System Stable", 0);
}

// ==============================================================================
// 8. ARDUINO MAIN LOOP
// ==============================================================================
void loop() {
  unsigned long currentMillis = millis();

  // 1. Maintain Wi-Fi Connection non-blockingly
  checkWiFiConnection();

  // 2. Poll Physical Sensors every 500ms
  if (currentMillis - lastSensorReadTime >= SENSOR_INTERVAL) {
    lastSensorReadTime = currentMillis;

    // A. Barometer / Altitude
    if (activePressureSensor == 1) {
      g_alt   = bme.readAltitude(1013.25);
      g_press = bme.readPressure() / 100.0F;
    } else if (activePressureSensor == 2) {
      g_alt   = bmp.readAltitude(1013.25);
      g_press = bmp.readPressure() / 100.0F;
    }

    // B. MPU6050 Motion / Vector
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    g_accX = a.acceleration.x;
    g_accY = a.acceleration.y;
    g_accZ = a.acceleration.z;

    // C. INA219 Voltage & Current
    g_volts = ina219.getBusVoltage_V();
    g_mA    = ina219.getCurrent_mA();

    // D. Temperatures
    float t_dht = dht.readTemperature();
    if (!isnan(t_dht)) g_cabinTemp = t_dht;

    ds18b20.requestTemperatures();
    float t_ds = ds18b20.getTempCByIndex(0);
    if (t_ds > -100.0 && t_ds < 80.0) g_outTemp = t_ds;

    // E. Terrain Alert Logic (Vertical baseline: X approx 0.0, Y approx -9.5)
    roughTerrainAlert = (abs(g_accX) > 3.0) || (abs(g_accY - (-9.5)) > 3.0);
    if (roughTerrainAlert) {
      displayMessage("ALERT!", "Rough Terrains", 3000);
    }
  }

  // 3. Audio & LCD Alert Management
  if (currentMillis < alertClearTime) {
    // Beep buzzer during active rough terrain alert
    if (lastLcdMsg == "ALERT!Rough Terrains") {
      digitalWrite(BUZZER, (currentMillis / 250) % 2);
    }
  } else {
    digitalWrite(BUZZER, LOW);
    displayMessage("Situation Normal", "System Stable", 0);
  }

  // 4. Cloud Command Checking every 1.0s
  if (currentMillis - lastCommandCheck >= COMMAND_INTERVAL) {
    lastCommandCheck = currentMillis;
    checkCloudCommands();
  }

  // 5. Cloud Telemetry Upload every 1.5s
  if (currentMillis - lastTelemetryUpload >= TELEMETRY_INTERVAL) {
    lastTelemetryUpload = currentMillis;
    uploadTelemetry();
  }

  // 6. Device Heartbeat every 10s
  if (currentMillis - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    lastHeartbeatTime = currentMillis;
    updateDeviceStatus(true);
  }
}
