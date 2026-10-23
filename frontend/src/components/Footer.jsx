import React from 'react';

export default function Footer({ deviceId, connectionHealth }) {
  return (
    <footer className="footer">
      Thermacore Telemetry Platform • Cloud Realtime Database Active • Device: {deviceId}
    </footer>
  );
}
