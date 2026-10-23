import React from 'react';

export default function StatusBanner({ telemetry, connectionHealth }) {
  if (connectionHealth === 'UNCONFIGURED') {
    return (
      <section className="status-banner banner-alert">
        <div className="status-left">
          <div className="status-icon" />
          <div>
            <div className="status-heading">Firebase Credentials Required</div>
            <div className="status-sub">
              Please copy <code>.env.example</code> to <code>.env.local</code> in the <code>frontend/</code> directory and populate your Firebase project configuration.
            </div>
          </div>
        </div>
        <span className="status-tag tag-alert">Action Required</span>
      </section>
    );
  }

  if (connectionHealth === 'OFFLINE' || !telemetry) {
    return (
      <section className="status-banner banner-alert">
        <div className="status-left">
          <div className="status-icon" />
          <div>
            <div className="status-heading">ESP32 Offline / Awaiting Stream</div>
            <div className="status-sub">
              No recent telemetry received from the ESP32. Check ESP32 power, Wi-Fi connection, and Serial Monitor.
            </div>
          </div>
        </div>
        <span className="status-tag tag-alert">Offline</span>
      </section>
    );
  }

  if (connectionHealth === 'STALE') {
    return (
      <section className="status-banner banner-stale">
        <div className="status-left">
          <div className="status-icon" />
          <div>
            <div className="status-heading">Telemetry Stale — Device Reconnecting</div>
            <div className="status-sub">
              Last sensor packet was received over 5 seconds ago. Displaying cached readings.
            </div>
          </div>
        </div>
        <span className="status-tag tag-stale">Telemetry Stale</span>
      </section>
    );
  }

  const ax = telemetry.accX ?? 0;
  const ay = telemetry.accY ?? -9.5;
  const voltsVal = telemetry.volts ?? 0;

  const isRoughAlert = (telemetry.alertTerrain === 1) || (Math.abs(ax) > 3.0) || (Math.abs(ay - (-9.5)) > 3.0);
  const isLowBat = (telemetry.alertBat === 1) || (voltsVal > 1.0 && voltsVal < 3.5);

  if (isRoughAlert || isLowBat) {
    const alerts = [];
    if (isRoughAlert) alerts.push('Rough Terrains');
    if (isLowBat) alerts.push(`Low Battery Volts (${voltsVal.toFixed(2)}V)`);

    return (
      <section className="status-banner banner-alert">
        <div className="status-left">
          <div className="status-icon" />
          <div>
            <div className="status-heading">Alert: {alerts.join(' & ')} Detected</div>
            <div className="status-sub">
              Buzzer active on GPIO 14 • Physical sensor threshold exceeded.
            </div>
          </div>
        </div>
        <span className="status-tag tag-alert">Critical Alert</span>
      </section>
    );
  }

  return (
    <section className="status-banner banner-stable">
      <div className="status-left">
        <div className="status-icon" />
        <div>
          <div className="status-heading">Situation Normal — System Stable</div>
          <div className="status-sub">
            All physical sensor readings are within normal operational limits • Buzzer: OFF
          </div>
        </div>
      </div>
      <span className="status-tag tag-stable">System Stable</span>
    </section>
  );
}
