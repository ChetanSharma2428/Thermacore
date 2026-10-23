import React, { useState, useEffect } from 'react';

export default function Header({ connectionHealth, lastTelemetryTime }) {
  const [currentTime, setCurrentTime] = useState('--:--:--');

  useEffect(() => {
    const timer = setInterval(() => {
      const d = new Date();
      setCurrentTime(d.toTimeString().split(' ')[0]);
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  const getStatusText = () => {
    switch (connectionHealth) {
      case 'LIVE':
        return 'Sensor Stream Live';
      case 'STALE':
        return 'Telemetry Stale (>5s)';
      case 'OFFLINE':
        return 'ESP32 Offline';
      case 'UNCONFIGURED':
        return 'Firebase Unconfigured';
      case 'CONNECTING':
      default:
        return 'Connecting to Cloud...';
    }
  };

  const getDotClass = () => {
    switch (connectionHealth) {
      case 'LIVE':
        return 'live-dot active';
      case 'STALE':
        return 'live-dot stale';
      case 'OFFLINE':
        return 'live-dot offline';
      default:
        return 'live-dot';
    }
  };

  return (
    <header className="header">
      <div className="brand">
        <div className="brand-logo">⚡</div>
        <div className="brand-text">
          <h1>Thermacore</h1>
          <p>Autonomous Thermal & Environmental Telemetry</p>
        </div>
      </div>
      <div className="header-status">
        <div className="status-badge">
          <div className={getDotClass()} />
          <span>{getStatusText()}</span>
        </div>
        <div className="status-badge" title={lastTelemetryTime ? `Last update: ${new Date(lastTelemetryTime).toLocaleTimeString()}` : ''}>
          🕒 {currentTime}
        </div>
      </div>
    </header>
  );
}
