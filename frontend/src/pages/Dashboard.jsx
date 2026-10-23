import React, { useState } from 'react';
import Header from '../components/Header';
import DeviceDock from '../components/DeviceDock';
import StatusBanner from '../components/StatusBanner';
import HardwareControls from '../components/HardwareControls';
import SensorGrid from '../components/SensorGrid';
import AttitudeIndicator from '../components/AttitudeIndicator';
import TemperatureTrendChart from '../components/TemperatureTrendChart';
import Footer from '../components/Footer';
import { useDeviceTelemetry } from '../hooks/useDeviceTelemetry';
import { useAuth } from '../hooks/useAuth';
import { sanitizeDeviceId } from '../lib/database';

const rawDefaultId = import.meta.env.VITE_DEFAULT_DEVICE_ID || 'thermacore-esp32-01';
const DEFAULT_DEVICE_ID = sanitizeDeviceId(rawDefaultId);

export default function Dashboard() {
  const [deviceId, setDeviceId] = useState(() => {
    try {
      const saved = localStorage.getItem('thermacore_cloud_device_id');
      const clean = sanitizeDeviceId(saved || DEFAULT_DEVICE_ID);
      localStorage.setItem('thermacore_cloud_device_id', clean);
      return clean;
    } catch {
      return DEFAULT_DEVICE_ID;
    }
  });

  const { user, authError } = useAuth();
  const {
    telemetry,
    deviceStatus,
    reportedState,
    dbError,
    lastTelemetryTime,
    connectionHealth,
    tempHistory,
    activeCommands,
    dispatchCommand
  } = useDeviceTelemetry(deviceId);

  const handleDeviceChange = (newDeviceId) => {
    const clean = sanitizeDeviceId(newDeviceId);
    setDeviceId(clean);
    try {
      localStorage.setItem('thermacore_cloud_device_id', clean);
    } catch (e) {
      console.warn('Failed to save deviceId to localStorage:', e);
    }
  };

  const isControlDisabled = connectionHealth === 'OFFLINE' || connectionHealth === 'UNCONFIGURED';

  return (
    <div className="container">
      {/* Header with live clock & connection badge */}
      <Header connectionHealth={connectionHealth} lastTelemetryTime={lastTelemetryTime} />

      {/* Cloud Device selection dock */}
      <DeviceDock
        deviceId={deviceId}
        onDeviceChange={handleDeviceChange}
        connectionHealth={connectionHealth}
        deviceStatus={deviceStatus}
      />

      {/* Auth or DB Error alerts */}
      {authError && (
        <div style={{ background: 'rgba(254, 242, 242, 0.95)', border: '1px solid #ef4444', borderRadius: '14px', padding: '12px 18px', marginBottom: '18px', fontSize: '13px', color: '#991b1b' }}>
          <strong>Authentication Error:</strong> {authError}
        </div>
      )}

      {dbError && (
        <div style={{ background: 'rgba(254, 242, 242, 0.95)', border: '1px solid #ef4444', borderRadius: '14px', padding: '12px 18px', marginBottom: '18px', fontSize: '13px', color: '#991b1b' }}>
          <strong>Database Permission / Connection Error:</strong> {dbError}
        </div>
      )}

      {/* Real-time Status Banner */}
      <StatusBanner telemetry={telemetry} connectionHealth={connectionHealth} />

      {/* Actuator Controls (Peltier & Vibration) */}
      <HardwareControls
        telemetry={telemetry}
        reportedState={reportedState}
        activeCommands={activeCommands}
        onSendCommand={dispatchCommand}
        disabled={isControlDisabled}
      />

      {/* Sensor Metric Cards */}
      <main>
        <SensorGrid telemetry={telemetry} />
        <AttitudeIndicator telemetry={telemetry} />
      </main>

      {/* Temperature Trend Chart */}
      <TemperatureTrendChart history={tempHistory} />

      {/* Footer */}
      <Footer deviceId={deviceId} connectionHealth={connectionHealth} />
    </div>
  );
}
