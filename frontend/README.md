# Thermacore Frontend (React + Vite)

Production-conscious React dashboard for the Thermacore IoT platform, migrated from the embedded ESP32 web server to a cloud-connected architecture backed by Firebase Realtime Database and Authentication.

## Features

- **Pixel-Accurate Glassmorphic Visuals**: Preserves the Premier White Glass Palette, responsive layout, status tags, and typography.
- **Real-Time Cloud Telemetry**: Replaces local HTTP polling with non-blocking Firebase Realtime Database SDK subscriptions (`onValue`).
- **Two-Way Actuator Confirmation**: Distinguishes requested state from authoritative hardware-reported state (`devices/{deviceId}/reportedState`).
- **Dynamic Connection & Stale-Data Health**: Automatically monitors device heartbeat and flags stale telemetry (>5s) or offline status (>15s).
- **Responsive 2D Attitude Indicator**: Real-time MPU6050 bubble visualization with safe threshold limits.
- **SVG Historical Temperature Chart**: Dual-channel history (Cabin vs Outside probe) with dynamic scaling.

## Quick Start

1. Install dependencies:
   ```bash
   npm install
   ```

2. Configure environment:
   ```bash
   cp .env.example .env.local
   ```
   Edit `.env.local` with your Firebase project credentials.

3. Run locally:
   ```bash
   npm run dev
   ```

4. Build for production:
   ```bash
   npm run build
   ```

## Directory Structure

```
frontend/
├── src/
│   ├── components/       # UI Components (Header, DeviceDock, Controls, SensorGrid, etc.)
│   ├── hooks/            # Reusable React hooks (useDeviceTelemetry, useAuth)
│   ├── lib/              # Firebase initialization, Auth, and RTDB database access
│   ├── pages/            # Dashboard page
│   ├── styles/           # Premier White Glass Palette stylesheets
│   ├── App.jsx           # App root
│   └── main.jsx          # Vite React entrypoint
├── .env.example          # Safe configuration template
└── package.json
```
