import { ref, onValue, set, serverTimestamp } from 'firebase/database';
import { rtdb, isConfigured } from './firebase';

/**
 * Subscribe to the latest real-time sensor telemetry for a device.
 * Path: devices/{deviceId}/telemetry
 */
export function subscribeToTelemetry(deviceId, onData, onError) {
  if (!isConfigured) {
    // Return no-op un-subscriber if Firebase is unconfigured
    return () => {};
  }
  const telemetryRef = ref(rtdb, `devices/${deviceId}/telemetry`);
  return onValue(
    telemetryRef,
    (snapshot) => {
      const val = snapshot.val();
      onData(val);
    },
    (err) => {
      console.error(`Telemetry subscription error for ${deviceId}:`, err);
      if (onError) onError(err);
    }
  );
}

/**
 * Subscribe to the device's connection status and heartbeat.
 * Path: devices/{deviceId}/status
 */
export function subscribeToStatus(deviceId, onData, onError) {
  if (!isConfigured) return () => {};
  const statusRef = ref(rtdb, `devices/${deviceId}/status`);
  return onValue(
    statusRef,
    (snapshot) => {
      const val = snapshot.val();
      onData(val);
    },
    (err) => {
      console.error(`Status subscription error for ${deviceId}:`, err);
      if (onError) onError(err);
    }
  );
}

/**
 * Subscribe to authoritative hardware-confirmed actuator state.
 * Path: devices/{deviceId}/reportedState
 */
export function subscribeToReportedState(deviceId, onData, onError) {
  if (!isConfigured) return () => {};
  const reportedRef = ref(rtdb, `devices/${deviceId}/reportedState`);
  return onValue(
    reportedRef,
    (snapshot) => {
      const val = snapshot.val();
      onData(val);
    },
    (err) => {
      console.error(`Reported state subscription error for ${deviceId}:`, err);
      if (onError) onError(err);
    }
  );
}

/**
 * Dispatch an actuator control command to the ESP32.
 * Path: devices/{deviceId}/commands
 * 
 * @param {string} deviceId 
 * @param {'peltier' | 'vibration'} actuator 
 * @param {number} requestedState (0=Off, 1=Heat, 2=Cool for Peltier; 0=Off, 1=On for Vib)
 */
export async function sendDeviceCommand(deviceId, actuator, requestedState) {
  if (!isConfigured) {
    throw new Error('Firebase is not configured. Add your credentials to .env.local');
  }

  const commandId = typeof crypto !== 'undefined' && crypto.randomUUID
    ? crypto.randomUUID()
    : 'cmd_' + Date.now() + '_' + Math.random().toString(36).substring(2, 8);

  const commandRef = ref(rtdb, `devices/${deviceId}/commands`);
  const payload = {
    commandId,
    actuator,
    requestedState,
    status: 'PENDING',
    timestamp: Date.now(),
    serverTime: serverTimestamp()
  };

  await set(commandRef, payload);
  return payload;
}
