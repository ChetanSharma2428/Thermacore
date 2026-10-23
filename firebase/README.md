# Firebase Architecture & Security Rules

This directory contains the Firebase Realtime Database rules and CLI configuration for the **Thermacore** IoT platform.

---

## 1. Security Architecture

The database structure isolates each device under its unique ID:

```
devices/
  └── {deviceId}/
        ├── telemetry/      # Latest sensor readings (read: users, write: device)
        ├── status/         # Heartbeat, local IP, firmware version
        ├── commands/       # Control requests (write: authorized users, read: device)
        └── reportedState/  # Hardware-confirmed state (write: device, read: users)
```

---

## 2. Security Rules Options

### A. Recommended Staged Rules (`database.rules.json`)
These rules require any authenticated user (including anonymous auth used by the frontend) to access the device nodes:

```json
{
  "rules": {
    "devices": {
      "$deviceId": {
        "telemetry": {
          ".read": "auth != null",
          ".write": "auth != null",
          ".validate": "newData.hasChildren(['timestamp'])"
        },
        "status": {
          ".read": "auth != null",
          ".write": "auth != null"
        },
        "commands": {
          ".read": "auth != null",
          ".write": "auth != null",
          ".validate": "newData.hasChildren(['commandId', 'actuator', 'requestedState'])"
        },
        "reportedState": {
          ".read": "auth != null",
          ".write": "auth != null"
        }
      }
    }
  }
}
```

### B. Prototype Test Rules (For immediate initial bring-up only)
If you want to verify your ESP32 Wi-Fi hardware before enabling Firebase Authentication:

```json
{
  "rules": {
    "devices": {
      "$deviceId": {
        ".read": true,
        ".write": true
      }
    }
  }
}
```
> **Warning**: Never leave these test rules active permanently. Once your ESP32 is communicating, switch to authenticated rules with a Database Secret or Auth Token.

---

## 3. Applying Rules in Firebase Console

1. Navigate to [Firebase Console](https://console.firebase.google.com).
2. Select your project -> **Build -> Realtime Database**.
3. Click on the **Rules** tab.
4. Paste the contents of `database.rules.json` and click **Publish**.

---

## 4. Local Testing with Firebase Emulator Suite (Optional)

If you have the Firebase CLI installed (`npm install -g firebase-tools`):

```bash
cd firebase
firebase emulators:start
```

Open the Emulator UI at `http://localhost:4000` to inspect Realtime Database reads and writes in real time without touching cloud quotas.
