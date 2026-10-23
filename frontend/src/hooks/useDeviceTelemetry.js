import { useState, useEffect, useRef, useCallback } from 'react';
import {
  subscribeToTelemetry,
  subscribeToStatus,
  subscribeToReportedState,
  sendDeviceCommand
} from '../lib/database';
import { isConfigured } from '../lib/firebase';

const STALE_THRESHOLD_MS = 5000;
const OFFLINE_THRESHOLD_MS = 15000;
const COMMAND_TIMEOUT_MS = 8000;

export function useDeviceTelemetry(deviceId) {
  const [telemetry, setTelemetry] = useState(null);
  const [deviceStatus, setDeviceStatus] = useState(null);
  const [reportedState, setReportedState] = useState(null);
  const [dbError, setDbError] = useState(null);
  const [lastTelemetryTime, setLastTelemetryTime] = useState(null);
  const [connectionHealth, setConnectionHealth] = useState('CONNECTING'); // CONNECTING | LIVE | STALE | OFFLINE | UNCONFIGURED
  const [tempHistory, setTempHistory] = useState([]); // [{ cab, out, time }]

  // Command tracking: { [commandId]: { actuator, requestedState, timestamp, status: 'PENDING' | 'CONFIRMED' | 'TIMEOUT' } }
  const [activeCommands, setActiveCommands] = useState({});

  const lastTelemTsRef = useRef(null);

  // 1. Telemetry subscription
  useEffect(() => {
    if (!isConfigured) {
      setConnectionHealth('UNCONFIGURED');
      return;
    }

    setConnectionHealth('CONNECTING');
    setDbError(null);

    const unsubTelem = subscribeToTelemetry(
      deviceId,
      (data) => {
        if (!data) return;
        const now = Date.now();
        lastTelemTsRef.current = now;
        setLastTelemetryTime(now);

        // Sanitize numeric fields
        const safeData = {
          alt: Number(data.alt),
          pres: Number(data.pres),
          cab: Number(data.cab),
          out: Number(data.out),
          volts: Number(data.volts),
          mA: Number(data.mA),
          accX: Number(data.accX),
          accY: Number(data.accY),
          accZ: Number(data.accZ),
          peltier: typeof data.peltier === 'number' ? data.peltier : 0,
          vib: typeof data.vib === 'number' ? data.vib : 0,
          alertTerrain: Number(data.alertTerrain) === 1 ? 1 : 0,
          alertBat: Number(data.alertBat) === 1 ? 1 : 0,
          timestamp: data.timestamp || now
        };

        setTelemetry(safeData);

        // Append to temperature history
        if (!isNaN(safeData.cab) && !isNaN(safeData.out)) {
          setTempHistory((prev) => {
            const next = [...prev, { cab: safeData.cab, out: safeData.out, time: now }];
            return next.length > 30 ? next.slice(next.length - 30) : next;
          });
        }
      },
      (err) => setDbError(err.message || 'Database error')
    );

    const unsubStatus = subscribeToStatus(
      deviceId,
      (data) => {
        if (data) setDeviceStatus(data);
      },
      (err) => setDbError(err.message || 'Status subscription error')
    );

    const unsubReported = subscribeToReportedState(
      deviceId,
      (data) => {
        if (!data) return;
        setReportedState(data);

        // Check if an active command was acknowledged
        if (data.lastCommandId) {
          setActiveCommands((prev) => {
            if (prev[data.lastCommandId]) {
              return {
                ...prev,
                [data.lastCommandId]: {
                  ...prev[data.lastCommandId],
                  status: 'CONFIRMED'
                }
              };
            }
            return prev;
          });
        }
      },
      (err) => setDbError(err.message || 'Reported state error')
    );

    return () => {
      unsubTelem();
      unsubStatus();
      unsubReported();
    };
  }, [deviceId]);

  // 2. Heartbeat & stale-data evaluation timer (every 1 second)
  useEffect(() => {
    if (!isConfigured) return;

    const interval = setInterval(() => {
      const now = Date.now();
      const last = lastTelemTsRef.current;

      if (!last) {
        setConnectionHealth('CONNECTING');
      } else if (now - last > OFFLINE_THRESHOLD_MS) {
        setConnectionHealth('OFFLINE');
      } else if (now - last > STALE_THRESHOLD_MS) {
        setConnectionHealth('STALE');
      } else {
        setConnectionHealth('LIVE');
      }

      // Check command timeouts
      setActiveCommands((prev) => {
        let changed = false;
        const updated = { ...prev };
        for (const [id, cmd] of Object.entries(updated)) {
          if (cmd.status === 'PENDING' && now - cmd.timestamp > COMMAND_TIMEOUT_MS) {
            updated[id] = { ...cmd, status: 'TIMEOUT' };
            changed = true;
          }
        }
        return changed ? updated : prev;
      });
    }, 1000);

    return () => clearInterval(interval);
  }, []);

  // 3. Command dispatcher
  const dispatchCommand = useCallback(
    async (actuator, requestedState) => {
      try {
        const cmd = await sendDeviceCommand(deviceId, actuator, requestedState);
        setActiveCommands((prev) => ({
          ...prev,
          [cmd.commandId]: {
            actuator,
            requestedState,
            timestamp: cmd.timestamp,
            status: 'PENDING'
          }
        }));
        return cmd;
      } catch (err) {
        console.error('Command dispatch failed:', err);
        throw err;
      }
    },
    [deviceId]
  );

  return {
    telemetry,
    deviceStatus,
    reportedState,
    dbError,
    lastTelemetryTime,
    connectionHealth,
    tempHistory,
    activeCommands,
    dispatchCommand
  };
}
