import React, { useState } from 'react';

export default function DeviceDock({ deviceId, onDeviceChange, connectionHealth, deviceStatus }) {
  const [inputVal, setInputVal] = useState(deviceId);

  const handleSubmit = (e) => {
    e.preventDefault();
    if (inputVal.trim()) {
      onDeviceChange(inputVal.trim());
    }
  };

  const getPillStyle = () => {
    if (connectionHealth === 'LIVE') {
      return {
        color: '#065f46',
        background: 'rgba(209, 250, 229, 0.9)',
        borderColor: '#10b981'
      };
    }
    if (connectionHealth === 'STALE') {
      return {
        color: '#92400e',
        background: 'rgba(254, 243, 199, 0.9)',
        borderColor: '#f59e0b'
      };
    }
    if (connectionHealth === 'OFFLINE') {
      return {
        color: '#991b1b',
        background: 'rgba(254, 242, 242, 0.9)',
        borderColor: '#ef4444'
      };
    }
    return {
      color: 'var(--text-muted)',
      background: 'rgba(255, 255, 255, 0.8)',
      borderColor: '#cbd5e1'
    };
  };

  const getPillText = () => {
    if (connectionHealth === 'LIVE') {
      return `🟢 Cloud Connected • ${deviceId} ${deviceStatus?.ip ? `(${deviceStatus.ip})` : ''}`;
    }
    if (connectionHealth === 'STALE') {
      return `🟡 Stale Telemetry • ${deviceId}`;
    }
    if (connectionHealth === 'OFFLINE') {
      return `🔴 Offline / Awaiting ESP32 Heartbeat • ${deviceId}`;
    }
    if (connectionHealth === 'UNCONFIGURED') {
      return `⚠️ Setup .env.local with Firebase Credentials`;
    }
    return `Connecting to ${deviceId}...`;
  };

  return (
    <div className="ip-config-bar">
      <form onSubmit={handleSubmit} style={{ display: 'flex', alignItems: 'center', gap: '12px', flexWrap: 'wrap' }}>
        <span style={{ fontWeight: 800, fontSize: '12px', color: 'var(--text-sub)', textTransform: 'uppercase', letterSpacing: '0.5px' }}>
          ☁️ Cloud Device ID:
        </span>
        <div className="ip-input-group">
          <input
            type="text"
            className="ip-input"
            value={inputVal}
            onChange={(e) => setInputVal(e.target.value)}
            placeholder="e.g. thermacore-esp32-01"
          />
          <button type="submit" className="ip-btn">SELECT DEVICE</button>
        </div>
      </form>
      <div
        style={{
          fontFamily: 'var(--font-mono)',
          fontSize: '11px',
          fontWeight: 700,
          border: '1px solid',
          padding: '5px 12px',
          borderRadius: '9999px',
          transition: 'all 0.2s',
          ...getPillStyle()
        }}
      >
        {getPillText()}
      </div>
    </div>
  );
}
