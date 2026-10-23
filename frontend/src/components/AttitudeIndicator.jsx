import React from 'react';

export default function AttitudeIndicator({ telemetry }) {
  const ax = telemetry && !isNaN(telemetry.accX) ? telemetry.accX : null;
  const ay = telemetry && !isNaN(telemetry.accY) ? telemetry.accY : null;
  const az = telemetry && !isNaN(telemetry.accZ) ? telemetry.accZ : null;

  const hasData = ax !== null && ay !== null && az !== null;

  // Baseline: X: 0.0, Y: -9.5 (Vertically mounted)
  const isRough = hasData && (
    telemetry.alertTerrain === 1 ||
    Math.abs(ax) > 3.0 ||
    Math.abs(ay - (-9.5)) > 3.0
  );

  // Attitude bubble clamp (10% to 90%)
  const posX = hasData ? Math.max(10, Math.min(90, 50 + (ax * 7))) : 50;
  const posY = hasData ? Math.max(10, Math.min(90, 50 - ((ay - (-9.5)) * 7))) : 50;

  return (
    <div className="attitude-card">
      <div className="sensor-header">
        <span className="sensor-name">Terrain & Tilt Stability (MPU6050)</span>
        <span
          className="sensor-badge"
          style={{
            color: hasData ? (isRough ? '#ef4444' : '#10b981') : 'var(--text-muted)',
            background: hasData ? (isRough ? 'rgba(239, 68, 68, 0.1)' : 'rgba(16, 185, 129, 0.1)') : 'rgba(148, 163, 184, 0.1)'
          }}
        >
          {hasData ? (isRough ? 'ROUGH TERRAIN' : 'STABLE') : 'INITIALIZING'}
        </span>
      </div>

      <div className="attitude-layout">
        <div>
          <div className="sensor-val-row">
            <span className="sensor-val" style={{ fontSize: '24px' }}>
              {hasData
                ? `X: ${ax.toFixed(2)} | Y: ${ay.toFixed(2)} | Z: ${az.toFixed(2)}`
                : 'X: -- | Y: -- | Z: --'}
            </span>
            <span className="sensor-unit">m/s²</span>
          </div>
          <p style={{ fontSize: '12px', color: 'var(--text-muted)', lineHeight: 1.5, marginTop: '6px' }}>
            Real-time MPU6050 accelerometer vector. Vertical baseline: (X: 0.0, Y: -9.5 m/s²).
            Alert triggers when axis displacement shifts by &gt; ±3.0 m/s².
          </p>
        </div>

        {/* Attitude Bubble Box */}
        <div className="attitude-view">
          <div className="attitude-axis-h" />
          <div className="attitude-axis-v" />
          <div className="attitude-safe-ring" />
          <div
            className={`attitude-bubble ${isRough ? 'alert' : ''}`}
            style={{
              top: `${posY}%`,
              left: `${posX}%`
            }}
          />
        </div>
      </div>
    </div>
  );
}
