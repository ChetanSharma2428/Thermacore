import { ref, onValue, set, serverTimestamp } from 'firebase/database';
import { rtdb, isConfigured } from './firebase';

/**
 * Sanitizes deviceId by stripping tabs, newlines, spaces, and illegal Firebase path characters (. # $ [ ])
 */
export function sanitizeDeviceId(rawId) {
  if (!rawId || typeof rawId !== 'string') return 'thermacore-esp32-01';
  const cleaned = rawId.replace(/[\t\r\n\s.#$[\]]/g, '').trim();
  return cleaned || 'thermacore-esp32-01';
}

/**
 * Subscribe to the latest real-time sensor telemetry for a device.
 * Path: devices/{deviceId}/telemetry
 */
export function subscribeToTelemetry(deviceId, onData, onError) {
  if (!isConfigured) {
    return () => {};
  }
  const cleanId = sanitizeDeviceId(deviceId);
  try {
    const telemetryRef = ref(rtdb, `devices/${cleanId}/telemetry`);
    return onValue(
      telemetryRef,
      (snapshot) => {
        const val = snapshot.val();
        onData(val);
      },
      (err) => {
        console.error(`Telemetry subscription error for ${cleanId}:`, err);
        if (onError) onError(err);
      }
    );
  } catch (err) {
    console.error(`Failed to create telemetry ref for ${cleanId}:`, err);
    if (onError) onError(err);
    return () => {};
  }
}

/**
 * Subscribe to the device's connection status and heartbeat.
 * Path: devices/{deviceId}/status
 */
export function subscribeToStatus(deviceId, onData, onError) {
  if (!isConfigured) return () => {};
  const cleanId = sanitizeDeviceId(deviceId);
  try {
    const statusRef = ref(rtdb, `devices/${cleanId}/status`);
    return onValue(
      statusRef,
      (snapshot) => {
        const val = snapshot.val();
        onData(val);
      },
      (err) => {
        console.error(`Status subscription error for ${cleanId}:`, err);
        if (onError) onError(err);
      }
    );
  } catch (err) {
    console.error(`Failed to create status ref for ${cleanId}:`, err);
    if (onError) onError(err);
    return () => {};
  }
}

/**
 * Subscribe to authoritative hardware-confirmed actuator state.
 * Path: devices/{deviceId}/reportedState
 */
export function subscribeToReportedState(deviceId, onData, onError) {
  if (!isConfigured) return () => {};
  const cleanId = sanitizeDeviceId(deviceId);
  try {
    const reportedRef = ref(rtdb, `devices/${cleanId}/reportedState`);
    return onValue(
      reportedRef,
      (snapshot) => {
        const val = snapshot.val();
        onData(val);
      },
      (err) => {
        console.error(`Reported state subscription error for ${cleanId}:`, err);
        if (onError) onError(err);
      }
    );
  } catch (err) {
    console.error(`Failed to create reportedState ref for ${cleanId}:`, err);
    if (onError) onError(err);
    return () => {};
  }
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
    throw new Error('Firebase is not configured. Add your credentials in Vercel or .env.local');
  }

  const cleanId = sanitizeDeviceId(deviceId);
  const commandId = typeof crypto !== 'undefined' && crypto.randomUUID
    ? crypto.randomUUID()
    : 'cmd_' + Date.now() + '_' + Math.random().toString(36).substring(2, 8);

  const commandRef = ref(rtdb, `devices/${cleanId}/commands`);
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
