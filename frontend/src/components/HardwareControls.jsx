import React, { useState } from 'react';

export default function HardwareControls({
  telemetry,
  reportedState,
  activeCommands,
  onSendCommand,
  disabled
}) {
  const [peltierPending, setPeltierPending] = useState(false);
  const [vibPending, setVibPending] = useState(false);
  const [actionMessage, setActionMessage] = useState('');

  // Use authoritative reported state first, falling back to telemetry
  const confirmedPeltier = reportedState?.peltier ?? telemetry?.peltier ?? 0;
  const confirmedVib = reportedState?.vibration ?? telemetry?.vib ?? 0;

  // Find most recent pending command for Peltier
  const latestPeltierCmd = Object.values(activeCommands)
    .filter((c) => c.actuator === 'peltier')
    .sort((a, b) => b.timestamp - a.timestamp)[0];

  // Find most recent pending command for Vibration
  const latestVibCmd = Object.values(activeCommands)
    .filter((c) => c.actuator === 'vibration')
    .sort((a, b) => b.timestamp - a.timestamp)[0];

  const handlePeltierChange = async (mode) => {
    if (disabled || peltierPending) return;
    setPeltierPending(true);
    const modeLabels = ['OFF', 'HEATING', 'COOLING'];
    setActionMessage(`Requesting Peltier: ${modeLabels[mode]}...`);
    try {
      await onSendCommand('peltier', mode);
      setActionMessage(`Command sent. Awaiting ESP32 hardware confirmation...`);
    } catch (err) {
      setActionMessage(`Error: ${err.message}`);
    } finally {
      setPeltierPending(false);
    }
  };

  const handleVibrationToggle = async () => {
    if (disabled || vibPending) return;
    const nextState = confirmedVib === 1 ? 0 : 1;
    setVibPending(true);
    setActionMessage(`Requesting Vibration: ${nextState === 1 ? 'ON' : 'OFF'}...`);
    try {
      await onSendCommand('vibration', nextState);
      setActionMessage(`Command sent. Awaiting ESP32 hardware confirmation...`);
    } catch (err) {
      setActionMessage(`Error: ${err.message}`);
    } finally {
      setVibPending(false);
    }
  };

  const getPeltierStatusText = () => {
    if (latestPeltierCmd?.status === 'PENDING') return 'SWITCHING...';
    if (confirmedPeltier === 1) return 'HEATING';
    if (confirmedPeltier === 2) return 'COOLING';
    return 'OFF';
  };

  const getPeltierStatusColor = () => {
    if (latestPeltierCmd?.status === 'PENDING') return '#f59e0b';
    if (confirmedPeltier === 1) return '#ef4444';
    if (confirmedPeltier === 2) return '#0284c7';
    return 'var(--text-muted)';
  };

  const getVibStatusText = () => {
    if (latestVibCmd?.status === 'PENDING') return 'SWITCHING...';
    if (confirmedVib === 1) return 'ACTIVE';
    return 'OFF';
  };

  const getVibStatusColor = () => {
    if (latestVibCmd?.status === 'PENDING') return '#f59e0b';
    if (confirmedVib === 1) return '#d97706';
    return 'var(--text-muted)';
  };

  return (
    <section className="controls-card">
      <div className="controls-title">⚙️ Hardware Controls</div>

      <div className="controls-grid">
        {/* Vibration Motor */}
        <div className="control-box">
          <div className="control-info">
            <div className="control-label">
              <span>Vibration Motor</span>
              <span style={{ fontSize: '12px', color: getVibStatusColor(), fontWeight: 800 }}>
                {getVibStatusText()}
              </span>
            </div>
            <div className="control-sub">Toggle mechanical actuator for surface snow/de-icing.</div>
            {latestVibCmd && (
              <div className="control-feedback" style={{ color: latestVibCmd.status === 'CONFIRMED' ? '#10b981' : latestVibCmd.status === 'TIMEOUT' ? '#ef4444' : '#f59e0b' }}>
                {latestVibCmd.status === 'PENDING' && '⏳ Awaiting ESP32 ACK...'}
                {latestVibCmd.status === 'CONFIRMED' && '✓ Confirmed by ESP32'}
                {latestVibCmd.status === 'TIMEOUT' && '⚠️ Command timed out (no ACK)'}
              </div>
            )}
          </div>

          <button
            className={`btn-toggle ${confirmedVib === 1 ? 'active-vib' : ''}`}
            onClick={handleVibrationToggle}
            disabled={disabled || vibPending}
          >
            <span>{confirmedVib === 1 ? 'Turn Vibration OFF' : 'Turn Vibration ON'}</span>
            <div className="btn-jewel" />
          </button>
        </div>

        {/* Peltier Module */}
        <div className="control-box">
          <div className="control-info">
            <div className="control-label">
              <span>Peltier Thermal Switch</span>
              <span style={{ fontSize: '12px', color: getPeltierStatusColor(), fontWeight: 800 }}>
                {getPeltierStatusText()}
              </span>
            </div>
            <div className="control-sub">Switch between cooling mode, heating mode, or off.</div>
            {latestPeltierCmd && (
              <div className="control-feedback" style={{ color: latestPeltierCmd.status === 'CONFIRMED' ? '#10b981' : latestPeltierCmd.status === 'TIMEOUT' ? '#ef4444' : '#f59e0b' }}>
                {latestPeltierCmd.status === 'PENDING' && '⏳ Awaiting ESP32 ACK...'}
                {latestPeltierCmd.status === 'CONFIRMED' && '✓ Confirmed by ESP32'}
                {latestPeltierCmd.status === 'TIMEOUT' && '⚠️ Command timed out (no ACK)'}
              </div>
            )}
          </div>

          <div className="peltier-tabs">
            <button
              className={`p-tab ${confirmedPeltier === 0 ? 'active-off' : ''}`}
              onClick={() => handlePeltierChange(0)}
              disabled={disabled || peltierPending}
            >
              OFF
            </button>
            <button
              className={`p-tab ${confirmedPeltier === 1 ? 'active-heat' : ''}`}
              onClick={() => handlePeltierChange(1)}
              disabled={disabled || peltierPending}
            >
              HEAT 🔥
            </button>
            <button
              className={`p-tab ${confirmedPeltier === 2 ? 'active-cool' : ''}`}
              onClick={() => handlePeltierChange(2)}
              disabled={disabled || peltierPending}
            >
              COOL ❄️
            </button>
          </div>
        </div>
      </div>

      {actionMessage && (
        <div style={{ marginTop: '14px', fontSize: '12px', color: 'var(--text-sub)', fontFamily: 'var(--font-mono)' }}>
          {actionMessage}
        </div>
      )}
    </section>
  );
}
