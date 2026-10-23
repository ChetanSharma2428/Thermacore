import React from 'react';

export default function SensorGrid({ telemetry }) {
  const cabVal = telemetry && !isNaN(telemetry.cab) ? telemetry.cab : null;
  const outVal = telemetry && !isNaN(telemetry.out) ? telemetry.out : null;
  const altVal = telemetry && !isNaN(telemetry.alt) ? telemetry.alt : null;
  const presVal = telemetry && !isNaN(telemetry.pres) ? telemetry.pres : null;
  const voltsVal = telemetry && !isNaN(telemetry.volts) ? telemetry.volts : null;
  const mAVal = telemetry && !isNaN(telemetry.mA) ? telemetry.mA : null;

  // Thermal Gradient calculation
  const deltaVal = cabVal !== null && outVal !== null ? (cabVal - outVal).toFixed(1) : null;

  // Cabin status
  const getCabStatus = () => {
    if (cabVal === null) return { text: 'Awaiting Sensor', color: 'var(--text-muted)' };
    if (cabVal < 0) return { text: 'Freezing Risk (<0°C)', color: '#ef4444' };
    if (cabVal < 10) return { text: 'Cold Enclosure', color: '#f59e0b' };
    return { text: 'Optimal (Normal)', color: '#10b981' };
  };

  // Battery status
  const getBatStatus = () => {
    if (voltsVal === null) return { text: 'Monitoring', color: 'var(--text-muted)' };
    const isLow = (telemetry?.alertBat === 1) || (voltsVal > 1.0 && voltsVal < 3.5);
    if (isLow) return { text: 'Low Battery (<3.5V)', color: '#ef4444' };
    return { text: 'Healthy', color: '#10b981' };
  };

  const cabStatus = getCabStatus();
  const batStatus = getBatStatus();

  return (
    <div className="sensors-grid">
      {/* Box 1: Cabin Temperature (DHT11) */}
      <div className="sensor-box">
        <div className="sensor-header">
          <span className="sensor-name">Cabin Temperature</span>
          <span className="sensor-badge">DHT11</span>
        </div>
        <div className="sensor-val-row">
          <span className="sensor-val">{cabVal !== null ? cabVal.toFixed(1) : '--'}</span>
          <span className="sensor-unit">°C</span>
        </div>
        <div className="sensor-footer">
          <span>Target Range</span>
          <span style={{ color: cabStatus.color }}>{cabStatus.text}</span>
        </div>
      </div>

      {/* Box 2: Outside Temperature (DS18B20) */}
      <div className="sensor-box">
        <div className="sensor-header">
          <span className="sensor-name">Outside Temperature</span>
          <span className="sensor-badge">DS18B20</span>
        </div>
        <div className="sensor-val-row">
          <span className="sensor-val">{outVal !== null ? outVal.toFixed(1) : '--'}</span>
          <span className="sensor-unit">°C</span>
        </div>
        <div className="sensor-footer">
          <span>External Probe</span>
          <span style={{ color: outVal !== null ? '#10b981' : 'var(--text-muted)' }}>
            {outVal !== null ? 'Live Sensor' : 'Awaiting Probe'}
          </span>
        </div>
      </div>

      {/* Box 3: Thermal Gradient (ΔT) */}
      <div className="sensor-box">
        <div className="sensor-header">
          <span className="sensor-name">Thermal Gradient (ΔT)</span>
          <span className="sensor-badge">DIFFERENTIAL</span>
        </div>
        <div className="sensor-val-row">
          <span className="sensor-val">
            {deltaVal !== null ? `${Number(deltaVal) >= 0 ? '+' : ''}${deltaVal}` : '--'}
          </span>
          <span className="sensor-unit">°C Δ</span>
        </div>
        <div className="sensor-footer">
          <span>Cabin vs Outside</span>
          <span style={{ color: deltaVal !== null && Number(deltaVal) > 10 ? '#10b981' : '#f59e0b' }}>
            {deltaVal !== null ? (Number(deltaVal) > 10 ? 'Shielded Active' : 'Equalized') : 'Calculating...'}
          </span>
        </div>
      </div>

      {/* Box 4: Altitude (BME280/BMP280) */}
      <div className="sensor-box">
        <div className="sensor-header">
          <span className="sensor-name">Altitude</span>
          <span className="sensor-badge">ALTIMETER</span>
        </div>
        <div className="sensor-val-row">
          <span className="sensor-val">{altVal !== null ? Math.round(altVal) : '--'}</span>
          <span className="sensor-unit">m</span>
        </div>
        <div className="sensor-footer">
          <span>Barometric Elevation</span>
          <span>1013.25 Reference</span>
        </div>
      </div>

      {/* Box 5: Atmospheric Pressure (BME280/BMP280) */}
      <div className="sensor-box">
        <div className="sensor-header">
          <span className="sensor-name">Barometric Pressure</span>
          <span className="sensor-badge">BAROMETER</span>
        </div>
        <div className="sensor-val-row">
          <span className="sensor-val">{presVal !== null ? presVal.toFixed(1) : '--'}</span>
          <span className="sensor-unit">hPa</span>
        </div>
        <div className="sensor-footer">
          <span>Ambient Pressure</span>
          <span>Live Barometer</span>
        </div>
      </div>

      {/* Box 6: Battery Voltage & Current (INA219) */}
      <div className="sensor-box">
        <div className="sensor-header">
          <span className="sensor-name">Battery Power</span>
          <span className="sensor-badge">INA219</span>
        </div>
        <div className="sensor-val-row">
          <span className="sensor-val">{voltsVal !== null ? voltsVal.toFixed(2) : '--'}</span>
          <span className="sensor-unit">V</span>
        </div>
        <div className="sensor-footer">
          <span>Current: {mAVal !== null ? `${mAVal.toFixed(1)} mA` : '-- mA'}</span>
          <span style={{ color: batStatus.color }}>{batStatus.text}</span>
        </div>
      </div>
    </div>
  );
}
