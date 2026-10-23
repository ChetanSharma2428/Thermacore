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
#include <WebServer.h>

// --- WI-FI CREDENTIALS ---
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- SENSOR PINS ---
#define DHTPIN 5
#define DHTTYPE DHT11
#define ONE_WIRE_BUS 4

// --- L298N ACTUATOR PINS ---
#define ENA 27
#define IN1 26 // Peltier Direction 1
#define IN2 25 // Peltier Direction 2
#define IN3 18 // Vibration Motor Direction 1
#define IN4 19 // Vibration Motor Direction 2
#define BUZZER 14

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_BME280 bme;
Adafruit_BMP280 bmp; 
Adafruit_MPU6050 mpu;
Adafruit_INA219 ina219;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
WebServer server(80);

// --- TIMERS & STATE TRACKING ---
unsigned long lastReadTime = 0;
unsigned long lcdClearTime = 0;
unsigned long alertClearTime = 0; 
String lastLcdMsg = "";

int activePressureSensor = 0; // 0=None, 1=BME280, 2=BMP280
int peltierState = 0;         // 0=Off, 1=Heating, 2=Cooling
int vibrationState = 0;       // 0=Off, 1=On
bool roughTerrainAlert = false;

// --- GLOBAL SENSOR VARIABLES ---
float g_alt = 0.0, g_press = 0.0;
float g_accX = 0.0, g_accY = -9.5, g_accZ = 0.0;
float g_volts = 0.0, g_mA = 0.0;
float g_cabinTemp = 0.0, g_outTemp = 0.0;

// LCD Message Helper
void displayMessage(String msgLine1, String msgLine2, int durationMs) {
  if (lastLcdMsg != msgLine1 + msgLine2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msgLine1);
    lcd.setCursor(0, 1);
    lcd.print(msgLine2);
    lastLcdMsg = msgLine1 + msgLine2;
  }
  if (durationMs > 0) alertClearTime = millis() + durationMs;
}

// Global CORS Header for cross-origin web app access
void sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Origin, Accept");
}

// OPTIONS preflight handler
void handleOptions() {
  sendCORS();
  server.send(204);
}

// --- EMBEDDED THERMACORE WEB DASHBOARD ---
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Thermacore</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;500;600;700;800;900&family=JetBrains+Mono:wght@500;600;700;800&display=swap');

    :root {
      --font-main: 'Plus Jakarta Sans', -apple-system, BlinkMacSystemFont, sans-serif;
      --font-mono: 'JetBrains Mono', monospace;

      /* Premier White Glass Palette */
      --glass-bg: rgba(255, 255, 255, 0.72);
      --glass-card: rgba(255, 255, 255, 0.78);
      --glass-border: rgba(255, 255, 255, 0.9);
      --glass-bevel: inset 0 1px 1px rgba(255, 255, 255, 0.95);
      --glass-shadow: 0 20px 40px -15px rgba(15, 23, 42, 0.06), 0 0 1px rgba(15, 23, 42, 0.05);

      --text-main: #0f172a;
      --text-sub: #475569;
      --text-muted: #94a3b8;
      
      --accent-blue: #0284c7;
      --accent-green: #10b981;
      --accent-red: #ef4444;
      --accent-amber: #f59e0b;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background-color: #f8fafc;
      background-image: 
        radial-gradient(at 10% 10%, rgba(224, 242, 254, 0.8) 0px, transparent 50%),
        radial-gradient(at 90% 15%, rgba(238, 242, 255, 0.7) 0px, transparent 50%),
        radial-gradient(at 50% 90%, rgba(241, 245, 249, 0.8) 0px, transparent 60%),
        linear-gradient(135deg, #f8fafc 0%, #edf2f7 100%);
      background-attachment: fixed;
      color: var(--text-main);
      font-family: var(--font-main);
      padding: 24px 20px;
      min-height: 100vh;
      -webkit-font-smoothing: antialiased;
    }

    .container { max-width: 1240px; margin: 0 auto; }

    /* HEADER */
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 16px 28px;
      background: var(--glass-bg);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      margin-bottom: 18px;
      box-shadow: var(--glass-shadow), var(--glass-bevel);
    }
    .brand { display: flex; align-items: center; gap: 14px; }
    .brand-logo {
      width: 44px;
      height: 44px;
      background: linear-gradient(135deg, #0284c7 0%, #0369a1 100%);
      border-radius: 12px;
      display: flex;
      align-items: center;
      justify-content: center;
      color: white;
      font-size: 22px;
      box-shadow: 0 8px 18px rgba(2, 132, 199, 0.3);
    }
    .brand-text h1 {
      font-size: 24px;
      font-weight: 900;
      letter-spacing: -0.5px;
      color: var(--text-main);
    }
    .brand-text p {
      font-size: 13px;
      color: var(--text-muted);
      font-weight: 500;
    }
    .header-status { display: flex; align-items: center; gap: 12px; }
    .status-badge {
      background: rgba(255, 255, 255, 0.85);
      border: 1px solid rgba(255, 255, 255, 0.9);
      padding: 8px 16px;
      border-radius: 9999px;
      font-size: 12px;
      font-weight: 700;
      color: var(--text-sub);
      display: flex;
      align-items: center;
      gap: 8px;
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.03);
    }
    .live-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #cbd5e1;
      transition: all 0.3s;
    }
    .live-dot.active {
      background: var(--accent-green);
      box-shadow: 0 0 10px var(--accent-green);
      animation: pulseDot 2s infinite;
    }
    @keyframes pulseDot { 0%, 100% { opacity: 1; transform: scale(1); } 50% { opacity: 0.4; transform: scale(0.85); } }

    /* HOST CONNECTION DOCK */
    .ip-config-bar {
      display: flex;
      background: var(--glass-bg);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
      border: 1px solid var(--glass-border);
      border-radius: 16px;
      padding: 12px 20px;
      margin-bottom: 20px;
      align-items: center;
      justify-content: space-between;
      flex-wrap: wrap;
      gap: 14px;
      box-shadow: var(--glass-shadow), var(--glass-bevel);
    }
    .ip-input-group { display: flex; align-items: center; gap: 8px; flex: 1; max-width: 420px; }
    .ip-input {
      flex: 1;
      padding: 9px 14px;
      border-radius: 10px;
      border: 1px solid #cbd5e1;
      font-family: var(--font-mono);
      font-size: 13px;
      background: rgba(255, 255, 255, 0.9);
      color: var(--text-main);
      box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.04);
    }
    .ip-input:focus { outline: none; border-color: #0284c7; box-shadow: 0 0 0 3px rgba(2, 132, 199, 0.15); }
    .ip-btn {
      padding: 9px 18px;
      background: linear-gradient(135deg, #0284c7 0%, #0369a1 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-weight: 800;
      font-size: 11px;
      letter-spacing: 0.5px;
      cursor: pointer;
      box-shadow: 0 4px 12px rgba(2, 132, 199, 0.25);
      transition: all 0.2s;
    }
    .ip-btn:hover { background: #0284c7; transform: translateY(-1px); }
    .ip-btn:active { transform: translateY(1px); }


    /* STATUS BANNER (GREEN STABLE / RED ALERT) */
    .status-banner {
      border-radius: 18px;
      padding: 20px 28px;
      margin-bottom: 24px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      transition: all 0.3s cubic-bezier(0.16, 1, 0.3, 1);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
    }
    .banner-stable {
      background: linear-gradient(135deg, rgba(236, 253, 245, 0.9) 0%, rgba(209, 250, 229, 0.75) 100%);
      border: 1.5px solid #10b981;
      box-shadow: 0 12px 30px -10px rgba(16, 185, 129, 0.25), var(--glass-bevel);
    }
    .banner-alert {
      background: linear-gradient(135deg, rgba(254, 242, 242, 0.95) 0%, rgba(254, 226, 226, 0.85) 100%);
      border: 2px solid #ef4444;
      box-shadow: 0 14px 35px -8px rgba(239, 68, 68, 0.35), var(--glass-bevel);
      animation: alertPulse 1.3s infinite ease-in-out;
    }
    @keyframes alertPulse {
      0%, 100% { border-color: #ef4444; box-shadow: 0 10px 25px rgba(239, 68, 68, 0.2); }
      50% { border-color: #dc2626; box-shadow: 0 16px 40px rgba(239, 68, 68, 0.4); }
    }
    .status-left { display: flex; align-items: center; gap: 16px; }
    .status-icon {
      width: 16px;
      height: 16px;
      border-radius: 50%;
      background: #10b981;
      box-shadow: 0 0 14px #10b981;
    }
    .banner-alert .status-icon {
      background: #ef4444;
      box-shadow: 0 0 16px #ef4444;
      animation: lampFlash 0.6s infinite;
    }
    @keyframes lampFlash { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }
    .status-heading {
      font-size: 19px;
      font-weight: 800;
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .banner-stable .status-heading { color: #065f46; }
    .banner-alert .status-heading { color: #991b1b; }
    .status-sub {
      font-size: 13px;
      font-weight: 500;
      color: var(--text-sub);
      margin-top: 2px;
    }
    .status-tag {
      font-size: 12px;
      font-weight: 800;
      padding: 6px 14px;
      border-radius: 9999px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .tag-stable { background: rgba(16, 185, 129, 0.15); border: 1px solid #10b981; color: #065f46; }
    .tag-alert { background: rgba(239, 68, 68, 0.15); border: 1.5px solid #ef4444; color: #991b1b; }

    /* CONTROLS SECTION */
    .controls-card {
      background: var(--glass-bg);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 24px;
      margin-bottom: 24px;
      box-shadow: var(--glass-shadow), var(--glass-bevel);
    }
    .controls-title {
      font-size: 13px;
      font-weight: 800;
      letter-spacing: 0.8px;
      color: var(--text-sub);
      text-transform: uppercase;
      margin-bottom: 18px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .controls-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 18px; }
    
    .control-box {
      background: var(--glass-card);
      border: 1px solid rgba(255, 255, 255, 0.95);
      border-radius: 16px;
      padding: 20px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      box-shadow: 0 4px 14px rgba(15, 23, 42, 0.02);
    }
    .control-info { margin-bottom: 16px; }
    .control-label {
      font-size: 15px;
      font-weight: 800;
      color: var(--text-main);
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .control-sub {
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 4px;
    }

    /* Buttons */
    .btn-toggle {
      background: linear-gradient(180deg, #ffffff 0%, #f1f5f9 100%);
      border: 1px solid #cbd5e1;
      border-radius: 12px;
      padding: 14px 18px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: space-between;
      font-size: 13px;
      font-weight: 800;
      color: var(--text-main);
      letter-spacing: 0.5px;
      transition: all 0.2s cubic-bezier(0.16, 1, 0.3, 1);
      box-shadow: 0 4px 12px rgba(15, 23, 42, 0.04), inset 0 1px 0 #ffffff;
    }
    .btn-toggle:hover { border-color: #94a3b8; transform: translateY(-1px); }
    .btn-toggle:active { transform: translateY(1px); }
    .btn-toggle.active-vib {
      background: linear-gradient(180deg, #f59e0b 0%, #d97706 100%);
      border-color: #d97706;
      color: white;
      box-shadow: 0 8px 24px rgba(245, 158, 11, 0.3), inset 0 1px 0 rgba(255, 255, 255, 0.4);
    }
    .btn-jewel {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: #cbd5e1;
      border: 1px solid #94a3b8;
    }
    .btn-toggle.active-vib .btn-jewel {
      background: #ffffff;
      border-color: #ffffff;
      box-shadow: 0 0 10px #ffffff;
    }

    /* Peltier 3-Way Selector */
    .peltier-tabs {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 6px;
      background: rgba(241, 245, 249, 0.9);
      padding: 5px;
      border-radius: 14px;
      border: 1px solid #e2e8f0;
    }
    .p-tab {
      padding: 11px;
      text-align: center;
      border-radius: 10px;
      font-size: 12px;
      font-weight: 800;
      color: var(--text-muted);
      background: transparent;
      border: none;
      cursor: pointer;
      transition: all 0.2s;
    }
    .p-tab.active-off {
      background: #ffffff;
      color: var(--text-main);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
    }
    .p-tab.active-heat {
      background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
      color: white;
      box-shadow: 0 4px 14px rgba(239, 68, 68, 0.35);
    }
    .p-tab.active-cool {
      background: linear-gradient(135deg, #0284c7 0%, #0369a1 100%);
      color: white;
      box-shadow: 0 4px 14px rgba(2, 132, 199, 0.35);
    }

    /* SENSOR BOXES GRID */
    .sensors-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 18px;
      margin-bottom: 24px;
    }
    .sensor-box {
      background: var(--glass-bg);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 22px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      box-shadow: var(--glass-shadow), var(--glass-bevel);
      transition: transform 0.2s, box-shadow 0.2s;
    }
    .sensor-box:hover { transform: translateY(-2px); box-shadow: 0 24px 45px -10px rgba(15, 23, 42, 0.1), var(--glass-bevel); }
    .sensor-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .sensor-name {
      font-size: 13px;
      font-weight: 800;
      color: var(--text-sub);
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .sensor-badge {
      font-size: 10px;
      font-weight: 800;
      padding: 3px 8px;
      border-radius: 6px;
      background: rgba(2, 132, 199, 0.1);
      color: #0284c7;
    }
    .sensor-val-row {
      display: flex;
      align-items: baseline;
      gap: 6px;
      margin-bottom: 12px;
    }
    .sensor-val {
      font-family: var(--font-mono);
      font-size: 38px;
      font-weight: 800;
      color: var(--text-main);
      letter-spacing: -0.5px;
      line-height: 1;
    }
    .sensor-unit {
      font-family: var(--font-mono);
      font-size: 16px;
      font-weight: 700;
      color: #0284c7;
    }
    .sensor-footer {
      border-top: 1px solid rgba(226, 232, 240, 0.8);
      padding-top: 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 12px;
      color: var(--text-muted);
      font-weight: 600;
    }

    /* ATTITUDE INDICATOR CARD */
    .attitude-card {
      grid-column: 1 / -1;
      background: var(--glass-bg);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 24px;
      box-shadow: var(--glass-shadow), var(--glass-bevel);
    }
    .attitude-layout {
      display: grid;
      grid-template-columns: 1fr 340px;
      gap: 24px;
      align-items: center;
    }
    @media (max-width: 860px) { .attitude-layout { grid-template-columns: 1fr; } }
    .attitude-view {
      height: 140px;
      background: linear-gradient(180deg, #e2e8f0 0%, #f1f5f9 100%);
      border: 1px solid #cbd5e1;
      border-radius: 16px;
      position: relative;
      overflow: hidden;
      box-shadow: inset 0 2px 6px rgba(0, 0, 0, 0.04);
    }
    .attitude-axis-h {
      position: absolute;
      left: 0; right: 0; top: 50%;
      height: 1.5px;
      background: rgba(2, 132, 199, 0.35);
    }
    .attitude-axis-v {
      position: absolute;
      top: 0; bottom: 0; left: 50%;
      width: 1.5px;
      background: rgba(2, 132, 199, 0.35);
    }
    .attitude-safe-ring {
      position: absolute;
      top: 50%; left: 50%;
      transform: translate(-50%, -50%);
      width: 80px;
      height: 80px;
      border: 1.5px dashed rgba(16, 185, 129, 0.6);
      border-radius: 50%;
      pointer-events: none;
    }
    .attitude-bubble {
      position: absolute;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: #0284c7;
      border: 2px solid white;
      box-shadow: 0 0 16px rgba(2, 132, 199, 0.6);
      transform: translate(-50%, -50%);
      transition: all 0.25s cubic-bezier(0.16, 1, 0.3, 1);
    }
    .attitude-bubble.alert {
      background: #ef4444;
      box-shadow: 0 0 20px rgba(239, 68, 68, 0.7);
    }

    /* TREND CHART */
    .chart-card {
      background: var(--glass-bg);
      backdrop-filter: blur(25px) saturate(180%);
      -webkit-backdrop-filter: blur(25px) saturate(180%);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 24px;
      margin-bottom: 24px;
      box-shadow: var(--glass-shadow), var(--glass-bevel);
    }
    .chart-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 14px;
    }
    .chart-title {
      font-size: 13px;
      font-weight: 800;
      color: var(--text-sub);
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .chart-legend { display: flex; gap: 16px; font-size: 12px; font-weight: 700; }
    .legend-item { display: flex; align-items: center; gap: 6px; }
    .legend-dot { width: 10px; height: 10px; border-radius: 50%; }

    .footer {
      text-align: center;
      font-size: 12px;
      color: var(--text-muted);
      padding: 10px 0 20px;
      font-weight: 600;
    }
  </style>
</head>
<body>
  <div class="container">
    
    <!-- HEADER -->
    <header class="header">
      <div class="brand">
        <div class="brand-logo">⚡</div>
        <div class="brand-text">
          <h1>Thermacore</h1>
          <p>Autonomous Thermal & Environmental Telemetry</p>
        </div>
      </div>
      <div class="header-status">
        <div class="status-badge">
          <div class="live-dot" id="liveDot"></div>
          <span id="connState">Connecting to Sensors...</span>
        </div>
        <div class="status-badge" id="liveTime">--:--:--</div>
      </div>
    </header>

    <!-- HARDWARE IP CONNECTION DOCK (Deployable across web/localhost) -->
    <div class="ip-config-bar" id="ipBar">
      <div style="display:flex; align-items:center; gap:12px; flex-wrap:wrap;">
        <span style="font-weight:800; font-size:12px; color:var(--text-sub); text-transform:uppercase; letter-spacing:0.5px;">📡 ESP32 Hardware IP:</span>
        <div class="ip-input-group">
          <input type="text" class="ip-input" id="espIpInput" placeholder="e.g. 192.168.1.50 or 192.168.4.1">
          <button class="ip-btn" onclick="saveEspIp()">CONNECT</button>
        </div>
      </div>
      <div id="ipStatusPill" style="font-family:var(--font-mono); font-size:11px; font-weight:700; color:var(--text-muted); background:rgba(255,255,255,0.8); border:1px solid #cbd5e1; padding:5px 12px; border-radius:9999px;">
        Awaiting IP Input
      </div>
    </div>

    <!-- Mixed Content HTTPS Warning (Shown only if browser blocks HTTP over HTTPS) -->
    <div id="httpsWarning" style="display:none; background:rgba(254, 242, 242, 0.95); border:1px solid #ef4444; border-radius:14px; padding:12px 18px; margin-bottom:18px; font-size:12px; color:#991b1b; line-height:1.5; box-shadow:var(--glass-shadow);">
      <strong>⚠️ Browser Security Note:</strong> Because this dashboard is hosted on <strong>HTTPS</strong>, your browser blocks requests to local <strong>HTTP (192.168.x.x)</strong> by default.
      <br>👉 <strong>Easy Fix in Chrome/Edge:</strong> Click the Padlock/Settings icon next to the address bar ➔ Click <strong>Site settings</strong> ➔ Set <strong>"Insecure content"</strong> to <strong>Allow</strong> ➔ Refresh!
    </div>


    <!-- STATUS BANNER (GREEN STABLE / RED ALERT) -->
    <section id="statusBanner" class="status-banner banner-stable">
      <div class="status-left">
        <div class="status-icon"></div>
        <div>
          <div class="status-heading" id="bannerHeading">Situation Normal — System Stable</div>
          <div class="status-sub" id="bannerSub">Awaiting live telemetry stream from ESP32 sensors...</div>
        </div>
      </div>
      <span id="bannerTag" class="status-tag tag-stable">System Stable</span>
    </section>

    <!-- CONTROLS PANEL -->
    <section class="controls-card">
      <div class="controls-title">⚙️ Hardware Controls</div>
      
      <div class="controls-grid">
        
        <!-- Vibration Motor -->
        <div class="control-box">
          <div class="control-info">
            <div class="control-label">
              <span>Vibration Motor</span>
              <span id="vibStateText" style="font-size:12px; color:var(--text-muted);">--</span>
            </div>
            <div class="control-sub">Toggle mechanical actuator for surface snow/de-icing.</div>
          </div>
          <button id="vibBtn" class="btn-toggle" onclick="toggleVibration()">
            <span id="vibBtnLabel">Turn Vibration ON</span>
            <div class="btn-jewel"></div>
          </button>
        </div>

        <!-- Peltier Module -->
        <div class="control-box">
          <div class="control-info">
            <div class="control-label">
              <span>Peltier Thermal Switch</span>
              <span id="peltierStateText" style="font-size:12px; color:var(--text-muted);">--</span>
            </div>
            <div class="control-sub">Switch between cooling mode, heating mode, or off.</div>
          </div>
          <div class="peltier-tabs">
            <button class="p-tab active-off" id="tabOff" onclick="setPeltier(0)">OFF</button>
            <button class="p-tab" id="tabHeat" onclick="setPeltier(1)">HEAT 🔥</button>
            <button class="p-tab" id="tabCool" onclick="setPeltier(2)">COOL ❄️</button>
          </div>
        </div>

      </div>
    </section>

    <!-- SENSOR VALUES (INDIVIDUAL BOX FORMAT - DIRECT FROM HARDWARE) -->
    <main class="sensors-grid">
      
      <!-- Box 1: Cabin Temperature (DHT11) -->
      <div class="sensor-box">
        <div class="sensor-header">
          <span class="sensor-name">Cabin Temperature</span>
          <span class="sensor-badge">DHT11</span>
        </div>
        <div class="sensor-val-row">
          <span class="sensor-val" id="valCab">--</span>
          <span class="sensor-unit">°C</span>
        </div>
        <div class="sensor-footer">
          <span>Target Range</span>
          <span id="cabStatus" style="color:var(--text-muted);">Awaiting Sensor</span>
        </div>
      </div>

      <!-- Box 2: Outside Temperature (DS18B20) -->
      <div class="sensor-box">
        <div class="sensor-header">
          <span class="sensor-name">Outside Temperature</span>
          <span class="sensor-badge">DS18B20</span>
        </div>
        <div class="sensor-val-row">
          <span class="sensor-val" id="valOut">--</span>
          <span class="sensor-unit">°C</span>
        </div>
        <div class="sensor-footer">
          <span>External Probe</span>
          <span id="outStatus">Live Sensor</span>
        </div>
      </div>

      <!-- Box 3: Thermal Gradient (ΔT) -->
      <div class="sensor-box">
        <div class="sensor-header">
          <span class="sensor-name">Thermal Gradient (ΔT)</span>
          <span class="sensor-badge">DIFFERENTIAL</span>
        </div>
        <div class="sensor-val-row">
          <span class="sensor-val" id="valDelta">--</span>
          <span class="sensor-unit">°C Δ</span>
        </div>
        <div class="sensor-footer">
          <span>Cabin vs Outside</span>
          <span id="deltaStatus" style="color:var(--text-muted);">Calculating...</span>
        </div>
      </div>

      <!-- Box 4: Altitude (BME280/BMP280) -->
      <div class="sensor-box">
        <div class="sensor-header">
          <span class="sensor-name">Altitude</span>
          <span class="sensor-badge">ALTIMETER</span>
        </div>
        <div class="sensor-val-row">
          <span class="sensor-val" id="valAlt">--</span>
          <span class="sensor-unit">m</span>
        </div>
        <div class="sensor-footer">
          <span>Barometric Elevation</span>
          <span>1013.25 Reference</span>
        </div>
      </div>

      <!-- Box 5: Atmospheric Pressure (BME280/BMP280) -->
      <div class="sensor-box">
        <div class="sensor-header">
          <span class="sensor-name">Barometric Pressure</span>
          <span class="sensor-badge">BAROMETER</span>
        </div>
        <div class="sensor-val-row">
          <span class="sensor-val" id="valPres">--</span>
          <span class="sensor-unit">hPa</span>
        </div>
        <div class="sensor-footer">
          <span>Ambient Pressure</span>
          <span>Live Barometer</span>
        </div>
      </div>

      <!-- Box 6: Battery Voltage & Current (INA219) -->
      <div class="sensor-box">
        <div class="sensor-header">
          <span class="sensor-name">Battery Power</span>
          <span class="sensor-badge">INA219</span>
        </div>
        <div class="sensor-val-row">
          <span class="sensor-val" id="valVolts">--</span>
          <span class="sensor-unit">V</span>
        </div>
        <div class="sensor-footer">
          <span id="valCurrent">Current: -- mA</span>
          <span id="batStatus" style="color:var(--text-muted);">Monitoring</span>
        </div>
      </div>

      <!-- Box 7: Terrain & Motion Stability (MPU6050) -->
      <div class="attitude-card">
        <div class="sensor-header">
          <span class="sensor-name">Terrain & Tilt Stability (MPU6050)</span>
          <span id="terrainBadge" class="sensor-badge" style="color:var(--text-muted);">INITIALIZING</span>
        </div>
        <div class="attitude-layout">
          <div>
            <div class="sensor-val-row">
              <span class="sensor-val" style="font-size:24px;" id="valAcc">X: -- | Y: -- | Z: --</span>
              <span class="sensor-unit">m/s²</span>
            </div>
            <p style="font-size:12px; color:var(--text-muted); line-height:1.5; margin-top:6px;">
              Real-time MPU6050 accelerometer vector. Vertical baseline: (X: 0.0, Y: -9.5 m/s²). Alert triggers when axis displacement shifts by > ±3.0 m/s².
            </p>
          </div>
          <!-- Attitude Bubble Box -->
          <div class="attitude-view">
            <div class="attitude-axis-h"></div>
            <div class="attitude-axis-v"></div>
            <div class="attitude-safe-ring"></div>
            <div class="attitude-bubble" id="attBubble" style="top:50%; left:50%;"></div>
          </div>
        </div>
      </div>

    </main>

    <!-- TEMPERATURE TREND CHART -->
    <section class="chart-card">
      <div class="chart-head">
        <span class="chart-title">Real-Time Sensor Temperature History</span>
        <div class="chart-legend">
          <div class="legend-item">
            <div class="legend-dot" style="background:#0284c7;"></div>
            <span style="color:#0284c7;">Cabin Temp (DHT11)</span>
          </div>
          <div class="legend-item">
            <div class="legend-dot" style="background:#d97706;"></div>
            <span style="color:#d97706;">Outside Temp (DS18B20)</span>
          </div>
        </div>
      </div>
      <svg viewBox="0 0 1000 120" style="width:100%; height:120px; overflow:visible;">
        <line x1="0" y1="20" x2="1000" y2="20" stroke="rgba(203, 213, 225, 0.5)" stroke-dasharray="4" />
        <line x1="0" y1="60" x2="1000" y2="60" stroke="rgba(203, 213, 225, 0.5)" stroke-dasharray="4" />
        <line x1="0" y1="100" x2="1000" y2="100" stroke="rgba(203, 213, 225, 0.5)" stroke-dasharray="4" />
        <polyline id="lineCab" fill="none" stroke="#0284c7" stroke-width="3" stroke-linecap="round" points="" />
        <polyline id="lineOut" fill="none" stroke="#d97706" stroke-width="3" stroke-linecap="round" points="" />
      </svg>
    </section>

    <footer class="footer">
      Thermacore Telemetry Platform • Direct Sensor Stream Active
    </footer>

  </div>

  <script>
    let vibState = 0;
    let peltierState = 0;
    let hasReceivedData = false;

    let histCab = [];
    let histOut = [];
    // Universal Connection Engine
    // Supports deployed web URLs (Vercel, Netlify, GitHub Pages) and Localhost
    let apiBase = '';
    const currentHost = window.location.hostname;
    const isDirectEsp = /^(192\.168\.|10\.|172\.(1[6-9]|2[0-9]|3[0-1])\.)/.test(currentHost);

    // Retrieve saved IP or default
    let savedIp = localStorage.getItem('thermacore_esp_ip') || (isDirectEsp ? window.location.origin : '');
    if (savedIp) {
      document.getElementById('espIpInput').value = savedIp.replace(/^https?:\/\//, '').replace(/\/+$/, '');
      apiBase = savedIp.startsWith('http') ? savedIp : 'http://' + savedIp;
      document.getElementById('ipStatusPill').innerText = 'Connecting to ' + document.getElementById('espIpInput').value + '...';
      document.getElementById('ipStatusPill').style.color = '#0284c7';
    }

    function saveEspIp() {
      let val = document.getElementById('espIpInput').value.trim();
      if (val) {
        val = val.replace(/^https?:\/\//, '').replace(/\/+$/, '');
        const fullUrl = 'http://' + val;
        localStorage.setItem('thermacore_esp_ip', fullUrl);
        apiBase = fullUrl;
        document.getElementById('ipStatusPill').innerText = 'Connecting to ' + val + '...';
        document.getElementById('ipStatusPill').style.color = '#0284c7';
        poll();
      }
    }

    // Live Clock
    setInterval(() => {
      const d = new Date();
      document.getElementById('liveTime').innerText = d.toTimeString().split(' ')[0];
    }, 1000);

    // Toggle Vibration Motor
    function toggleVibration() {
      fetch(apiBase + '/vibration', { method: 'POST' })
        .then(res => res.text())
        .then(val => {
          vibState = (val === '1' || val === 'ON') ? 1 : (val === '0' || val === 'OFF' ? 0 : (vibState ? 0 : 1));
          renderVib(vibState);
        })
        .catch(err => console.error('Error triggering vibration:', err));
    }

    function renderVib(st) {
      const btn = document.getElementById('vibBtn');
      const lbl = document.getElementById('vibBtnLabel');
      const txt = document.getElementById('vibStateText');
      if (st == 1) {
        btn.className = 'btn-toggle active-vib';
        lbl.innerText = 'Turn Vibration OFF';
        txt.innerText = 'ACTIVE';
        txt.style.color = '#d97706';
      } else {
        btn.className = 'btn-toggle';
        lbl.innerText = 'Turn Vibration ON';
        txt.innerText = 'OFF';
        txt.style.color = 'var(--text-muted)';
      }
    }

    // Set Peltier Mode
    function setPeltier(mode) {
      fetch(apiBase + '/peltier', { method: 'POST' })
        .then(res => res.text())
        .then(val => {
          peltierState = parseInt(val);
          if (isNaN(peltierState)) peltierState = mode;
          renderPeltier(peltierState);
        })
        .catch(err => console.error('Error setting peltier:', err));
    }

    function renderPeltier(mode) {
      const tOff = document.getElementById('tabOff');
      const tHeat = document.getElementById('tabHeat');
      const tCool = document.getElementById('tabCool');
      const txt = document.getElementById('peltierStateText');

      tOff.className = 'p-tab';
      tHeat.className = 'p-tab';
      tCool.className = 'p-tab';

      if (mode == 1) {
        tHeat.className = 'p-tab active-heat';
        txt.innerText = 'HEATING';
        txt.style.color = '#ef4444';
      } else if (mode == 2) {
        tCool.className = 'p-tab active-cool';
        txt.innerText = 'COOLING';
        txt.style.color = '#0284c7';
      } else {
        tOff.className = 'p-tab active-off';
        txt.innerText = 'OFF';
        txt.style.color = 'var(--text-muted)';
      }
    }

    // Live Telemetry Polling from Physical ESP32 Sensors
    function poll() {
      if (!apiBase && !isDirectEsp) {
        document.getElementById('connState').innerText = 'Enter ESP32 IP';
        document.getElementById('ipStatusPill').innerText = 'Awaiting IP Address';
        document.getElementById('ipStatusPill').style.color = 'var(--text-muted)';
        return;
      }

      fetch(apiBase + '/data')
        .then(res => {
          if (!res.ok) throw new Error('HTTP ' + res.status);
          return res.json();
        })
        .then(d => {
          hasReceivedData = true;
          document.getElementById('connState').innerText = 'Sensor Stream Live';
          document.getElementById('liveDot').className = 'live-dot active';
          
          const cleanHost = apiBase ? apiBase.replace(/^https?:\/\//, '') : window.location.host;
          document.getElementById('ipStatusPill').innerText = '🟢 Connected to ' + cleanHost;
          document.getElementById('ipStatusPill').style.color = '#065f46';
          document.getElementById('ipStatusPill').style.background = 'rgba(209, 250, 229, 0.9)';
          document.getElementById('ipStatusPill').style.borderColor = '#10b981';

          // Hide HTTPS warning if connection is successful
          document.getElementById('httpsWarning').style.display = 'none';

          updateSensorUI(d);
        })
        .catch(err => {
          if (!hasReceivedData) {
            document.getElementById('connState').innerText = 'Searching for ESP32...';
            document.getElementById('liveDot').className = 'live-dot';
            document.getElementById('ipStatusPill').innerText = '🔴 Offline / Check IP';
            document.getElementById('ipStatusPill').style.color = '#991b1b';
            document.getElementById('ipStatusPill').style.background = 'rgba(254, 242, 242, 0.9)';
            document.getElementById('ipStatusPill').style.borderColor = '#ef4444';

            // If on HTTPS, display helpful Mixed Content instructions
            if (window.location.protocol === 'https:') {
              document.getElementById('httpsWarning').style.display = 'block';
            }
          }
        });
    }


    function updateSensorUI(d) {
      // 1. Cabin Temp (DHT11)
      const cabVal = Number(d.cab);
      if (!isNaN(cabVal)) {
        document.getElementById('valCab').innerText = cabVal.toFixed(1);
        const cabStatus = document.getElementById('cabStatus');
        if (cabVal < 0) {
          cabStatus.innerText = 'Freezing Risk (<0°C)';
          cabStatus.style.color = '#ef4444';
        } else if (cabVal < 10) {
          cabStatus.innerText = 'Cold Enclosure';
          cabStatus.style.color = '#f59e0b';
        } else {
          cabStatus.innerText = 'Optimal (Normal)';
          cabStatus.style.color = '#10b981';
        }
      }

      // 2. Outside Temp (DS18B20)
      const outVal = Number(d.out);
      if (!isNaN(outVal)) {
        document.getElementById('valOut').innerText = outVal.toFixed(1);
      }

      // 3. Thermal Gradient (Delta)
      if (!isNaN(cabVal) && !isNaN(outVal)) {
        const delta = (cabVal - outVal).toFixed(1);
        document.getElementById('valDelta').innerText = (delta >= 0 ? '+' : '') + delta;
        document.getElementById('deltaStatus').innerText = delta > 10 ? 'Shielded Active' : 'Equalized';
        document.getElementById('deltaStatus').style.color = delta > 10 ? '#10b981' : '#f59e0b';
      }

      // 4. Altitude (BME280/BMP280)
      const altVal = Number(d.alt);
      if (!isNaN(altVal)) {
        document.getElementById('valAlt').innerText = Math.round(altVal);
      }

      // 5. Pressure (BME280/BMP280)
      const presVal = Number(d.pres);
      if (!isNaN(presVal)) {
        document.getElementById('valPres').innerText = presVal.toFixed(1);
      }

      // 6. Battery Power (INA219)
      const voltsVal = Number(d.volts);
      const mAVal = Number(d.mA);
      if (!isNaN(voltsVal)) {
        document.getElementById('valVolts').innerText = voltsVal.toFixed(2);
      }
      if (!isNaN(mAVal)) {
        document.getElementById('valCurrent').innerText = `Current: ${mAVal.toFixed(1)} mA`;
      }

      // 7. Accel & Tilt (MPU6050)
      const ax = Number(d.accX);
      const ay = Number(d.accY);
      const az = Number(d.accZ);
      if (!isNaN(ax) && !isNaN(ay) && !isNaN(az)) {
        document.getElementById('valAcc').innerText = `X: ${ax.toFixed(2)} | Y: ${ay.toFixed(2)} | Z: ${az.toFixed(2)}`;
        
        // Tilt bubble position (Baseline: X: 0, Y: -9.5)
        const bubble = document.getElementById('attBubble');
        const posX = Math.max(10, Math.min(90, 50 + (ax * 7)));
        const posY = Math.max(10, Math.min(90, 50 - ((ay - (-9.5)) * 7)));
        bubble.style.left = posX + '%';
        bubble.style.top = posY + '%';

        const isRough = (d.alertTerrain === 1) || (Math.abs(ax) > 3.0) || (Math.abs(ay - (-9.5)) > 3.0);
        bubble.className = isRough ? 'attitude-bubble alert' : 'attitude-bubble';

        const tBadge = document.getElementById('terrainBadge');
        if (isRough) {
          tBadge.innerText = 'ROUGH TERRAIN';
          tBadge.style.color = '#ef4444';
        } else {
          tBadge.innerText = 'STABLE';
          tBadge.style.color = '#10b981';
        }
      }

      // Battery Alert Check
      const isLowBat = (d.alertBat === 1) || (voltsVal > 1.0 && voltsVal < 3.5);
      const batStatus = document.getElementById('batStatus');
      if (isLowBat) {
        batStatus.innerText = 'Low Battery (<3.5V)';
        batStatus.style.color = '#ef4444';
      } else {
        batStatus.innerText = 'Healthy';
        batStatus.style.color = '#10b981';
      }

      // Master Status Banner
      const isRoughAlert = (d.alertTerrain === 1) || (Math.abs(ax) > 3.0) || (Math.abs(ay - (-9.5)) > 3.0);
      const banner = document.getElementById('statusBanner');
      const heading = document.getElementById('bannerHeading');
      const sub = document.getElementById('bannerSub');
      const tag = document.getElementById('bannerTag');

      if (isRoughAlert || isLowBat) {
        banner.className = 'status-banner banner-alert';
        let alerts = [];
        if (isRoughAlert) alerts.push('Rough Terrains');
        if (isLowBat) alerts.push(`Low Battery Volts (${voltsVal.toFixed(2)}V)`);

        heading.innerText = `Alert: ${alerts.join(' & ')} Detected`;
        sub.innerText = 'Buzzer active on GPIO 14 • Physical sensor threshold exceeded.';
        tag.className = 'status-tag tag-alert';
        tag.innerText = 'Critical Alert';
      } else {
        banner.className = 'status-banner banner-stable';
        heading.innerText = 'Situation Normal — System Stable';
        sub.innerText = 'All physical sensor readings are within normal operational limits • Buzzer: OFF';
        tag.className = 'status-tag tag-stable';
        tag.innerText = 'System Stable';
      }

      // Actuators Sync
      if (d.vib !== undefined) renderVib(d.vib);
      if (d.peltier !== undefined) renderPeltier(d.peltier);

      // Append to graph
      if (!isNaN(cabVal) && !isNaN(outVal)) {
        histCab.push(cabVal);
        if (histCab.length > 30) histCab.shift();
        histOut.push(outVal);
        if (histOut.length > 30) histOut.shift();
        paintGraph();
      }
    }

    function paintGraph() {
      if (histCab.length < 2) return;
      const allVals = [...histCab, ...histOut];
      const min = Math.min(...allVals) - 2;
      const max = Math.max(...allVals) + 2;
      const span = (max - min) || 1;
      const w = 1000, h = 120;

      const ptsCab = histCab.map((v, i) => {
        const x = (i / (histCab.length - 1)) * w;
        const y = h - ((v - min) / span) * h;
        return `${x.toFixed(1)},${y.toFixed(1)}`;
      }).join(' ');

      const ptsOut = histOut.map((v, i) => {
        const x = (i / (histOut.length - 1)) * w;
        const y = h - ((v - min) / span) * h;
        return `${x.toFixed(1)},${y.toFixed(1)}`;
      }).join(' ');

      document.getElementById('lineCab').setAttribute('points', ptsCab);
      document.getElementById('lineOut').setAttribute('points', ptsOut);
    }

    // Start polling physical sensor data every 500ms
    setInterval(poll, 500);
    poll();
  </script>
</body>
</html>

)rawliteral";

// --- WEB SERVER ENDPOINTS ---

// Root: Simple JSON status check
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", htmlPage);
}

// GET /data: Streams all live sensor telemetry
void handleData() {
  sendCORS();
  String json = "{";
  json += "\"alt\":" + String(g_alt, 1) + ",";
  json += "\"pres\":" + String(g_press, 1) + ",";
  json += "\"cab\":" + String(g_cabinTemp, 1) + ",";
  json += "\"out\":" + String(g_outTemp, 1) + ",";
  json += "\"volts\":" + String(g_volts, 2) + ",";
  json += "\"mA\":" + String(g_mA, 1) + ",";
  json += "\"accX\":" + String(g_accX, 2) + ",";
  json += "\"accY\":" + String(g_accY, 2) + ",";
  json += "\"accZ\":" + String(g_accZ, 2) + ",";
  json += "\"peltier\":" + String(peltierState) + ",";
  json += "\"vib\":" + String(vibrationState) + ",";
  json += "\"alertTerrain\":" + String(roughTerrainAlert ? 1 : 0) + ",";
  json += "\"alertBat\":" + String((g_volts < 3.5 && g_volts > 1.0) ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}

// POST /peltier: Switches or cycles Peltier mode
void handlePeltier() {
  sendCORS();
  if (server.hasArg("mode")) {
    peltierState = server.arg("mode").toInt();
  } else {
    peltierState = (peltierState + 1) % 3; // Cycles 0 -> 1 -> 2 -> 0
  }
  
  if (peltierState == 0) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    displayMessage("Peltier: OFF", "Manual Override", 3000);
  } else if (peltierState == 1) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); // Heat
    displayMessage("HEATING MODE", "Manual Override", 3000);
  } else {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); // Cool
    displayMessage("COOLING MODE", "Manual Override", 3000);
  }
  server.send(200, "text/plain", String(peltierState));
}

// POST /vibration: Toggles Vibration motor
void handleVibration() {
  sendCORS();
  if (server.hasArg("state")) {
    vibrationState = server.arg("state").toInt();
  } else {
    vibrationState = !vibrationState;
  }

  if (vibrationState == 1) {
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    displayMessage("VIBRATION: ON", "De-Icing Active", 3000);
  } else {
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    displayMessage("VIBRATION: OFF", "De-Icing Stopped", 3000);
  }
  server.send(200, "text/plain", String(vibrationState));
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Initialize Pin Modes
  pinMode(ENA, OUTPUT); 
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); 
  pinMode(BUZZER, OUTPUT);

  // Default Pin States
  analogWrite(ENA, 255); 
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  digitalWrite(BUZZER, LOW);

  // LCD Setup
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("THERMACORE INIT");
  lcd.setCursor(0, 1); lcd.print("CONNECTING WIFI.");

  // Wi-Fi Connection
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) { 
    delay(500); 
    Serial.print(".");
    attempts++;
  }
  
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0); lcd.print("WIFI CONNECTED");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP().toString());
    Serial.println("\n\n=== THERMACORE ESP32 ACTIVE ===");
    Serial.print("Local IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Enter this IP into your deployed Thermacore web dashboard!");
  } else {
    // Fallback Hotspot if Wi-Fi cannot connect
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Thermacore-ESP32", "12345678");
    lcd.setCursor(0, 0); lcd.print("HOTSPOT ACTIVE");
    lcd.setCursor(0, 1); lcd.print(WiFi.softAPIP().toString());
    Serial.println("\nWiFi Failed. Starting Hotspot 'Thermacore-ESP32'");
    Serial.print("Connect to hotspot and use IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // Init Sensors
  dht.begin(); 
  ds18b20.begin(); 
  mpu.begin(); 
  ina219.begin();

  if (bme.begin(0x76) || bme.begin(0x77)) activePressureSensor = 1;
  else if (bmp.begin(0x76) || bmp.begin(0x77)) activePressureSensor = 2;

  // Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/data", HTTP_OPTIONS, handleOptions);
  server.on("/peltier", HTTP_POST, handlePeltier);
  server.on("/peltier", HTTP_OPTIONS, handleOptions);
  server.on("/vibration", HTTP_POST, handleVibration);
  server.on("/vibration", HTTP_OPTIONS, handleOptions);
  server.begin();
  
  delay(2000);
}

// --- MAIN LOOP ---
void loop() {
  server.handleClient(); // Process incoming web requests
  unsigned long currentMillis = millis();

  // --- SENSOR POLLING (Every 500ms) ---
  if (currentMillis - lastReadTime >= 500) {
    lastReadTime = currentMillis;

    // 1. Altitude & Barometer
    if (activePressureSensor == 1) { 
      g_alt = bme.readAltitude(1013.25); 
      g_press = bme.readPressure() / 100.0F; 
    } else if (activePressureSensor == 2) { 
      g_alt = bmp.readAltitude(1013.25); 
      g_press = bmp.readPressure() / 100.0F; 
    }
    
    // 2. MPU6050 Motion / Vibration
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    g_accX = a.acceleration.x; 
    g_accY = a.acceleration.y; 
    g_accZ = a.acceleration.z;
    
    // 3. INA219 Battery Telemetry
    g_volts = ina219.getBusVoltage_V(); 
    g_mA = ina219.getCurrent_mA();

    // 4. Temperatures
    float t_dht = dht.readTemperature();
    if (!isnan(t_dht)) g_cabinTemp = t_dht;

    ds18b20.requestTemperatures(); 
    float t_ds = ds18b20.getTempCByIndex(0);
    if (t_ds > -100.0 && t_ds < 80.0) g_outTemp = t_ds;

    // --- MPU6050 TILT & ROUGH TERRAIN LOGIC ---
    // Baseline: X approx 0.0, Y approx -9.5 (Vertically mounted)
    roughTerrainAlert = (abs(g_accX) > 3.0) || (abs(g_accY - (-9.5)) > 3.0);
    if (roughTerrainAlert) {
      displayMessage("ALERT!", "Rough Terrains", 3000);
    }
  }

  // --- LCD & ALERTS CLEANUP ---
  if (currentMillis < alertClearTime) {
    // Beep buzzer for Rough Terrains alert
    if (lastLcdMsg == "ALERT!Rough Terrains") {
      digitalWrite(BUZZER, (currentMillis / 250) % 2); 
    }
  } else {
    digitalWrite(BUZZER, LOW);
    displayMessage("Situation Normal", "System Stable", 0);
  }
}
