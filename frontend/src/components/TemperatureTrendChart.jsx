import React from 'react';

export default function TemperatureTrendChart({ history }) {
  const w = 1000;
  const h = 120;

  // Need at least 2 points to draw lines
  if (!history || history.length < 2) {
    return (
      <section className="chart-card">
        <div className="chart-head">
          <span className="chart-title">Real-Time Sensor Temperature History</span>
          <div className="chart-legend">
            <div className="legend-item">
              <div className="legend-dot" style={{ background: '#0284c7' }} />
              <span style={{ color: '#0284c7' }}>Cabin Temp (DHT11)</span>
            </div>
            <div className="legend-item">
              <div className="legend-dot" style={{ background: '#d97706' }} />
              <span style={{ color: '#d97706' }}>Outside Temp (DS18B20)</span>
            </div>
          </div>
        </div>
        <div style={{ height: '120px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: 'var(--text-muted)', fontSize: '13px' }}>
          Collecting temperature data points...
        </div>
      </section>
    );
  }

  const cabVals = history.map((pt) => pt.cab).filter((v) => !isNaN(v));
  const outVals = history.map((pt) => pt.out).filter((v) => !isNaN(v));
  const allVals = [...cabVals, ...outVals];

  const min = Math.min(...allVals) - 2;
  const max = Math.max(...allVals) + 2;
  const span = max - min || 1;

  const pointsCab = history
    .map((pt, i) => {
      const x = (i / (history.length - 1)) * w;
      const y = h - ((pt.cab - min) / span) * h;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  const pointsOut = history
    .map((pt, i) => {
      const x = (i / (history.length - 1)) * w;
      const y = h - ((pt.out - min) / span) * h;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  return (
    <section className="chart-card">
      <div className="chart-head">
        <span className="chart-title">Real-Time Sensor Temperature History</span>
        <div className="chart-legend">
          <div className="legend-item">
            <div className="legend-dot" style={{ background: '#0284c7' }} />
            <span style={{ color: '#0284c7' }}>Cabin Temp (DHT11)</span>
          </div>
          <div className="legend-item">
            <div className="legend-dot" style={{ background: '#d97706' }} />
            <span style={{ color: '#d97706' }}>Outside Temp (DS18B20)</span>
          </div>
        </div>
      </div>

      <svg viewBox={`0 0 ${w} ${h}`} style={{ width: '100%', height: '120px', overflow: 'visible' }}>
        <line x1="0" y1="20" x2={w} y2="20" stroke="rgba(203, 213, 225, 0.5)" strokeDasharray="4" />
        <line x1="0" y1="60" x2={w} y2="60" stroke="rgba(203, 213, 225, 0.5)" strokeDasharray="4" />
        <line x1="0" y1="100" x2={w} y2="100" stroke="rgba(203, 213, 225, 0.5)" strokeDasharray="4" />
        <polyline
          fill="none"
          stroke="#0284c7"
          strokeWidth="3"
          strokeLinecap="round"
          strokeLinejoin="round"
          points={pointsCab}
        />
        <polyline
          fill="none"
          stroke="#d97706"
          strokeWidth="3"
          strokeLinecap="round"
          strokeLinejoin="round"
          points={pointsOut}
        />
      </svg>
    </section>
  );
}
