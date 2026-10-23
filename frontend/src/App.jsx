import React from 'react';
import Dashboard from './pages/Dashboard';

class ErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error };
  }

  componentDidCatch(error, errorInfo) {
    console.error('Thermacore uncaught render error:', error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div style={{ maxWidth: '600px', margin: '40px auto', padding: '24px', background: '#fff', borderRadius: '16px', boxShadow: '0 10px 25px rgba(0,0,0,0.05)', fontFamily: 'system-ui, sans-serif' }}>
          <h2 style={{ color: '#ef4444', marginBottom: '12px' }}>⚠️ Dashboard Render Error</h2>
          <p style={{ color: '#475569', fontSize: '14px', lineHeight: 1.5, marginBottom: '16px' }}>
            {this.state.error?.message || 'An unexpected error occurred while loading the dashboard.'}
          </p>
          <button
            onClick={() => {
              localStorage.clear();
              window.location.reload();
            }}
            style={{ padding: '10px 18px', background: '#0284c7', color: '#fff', border: 'none', borderRadius: '8px', cursor: 'pointer', fontWeight: 600 }}
          >
            Reset Cached State & Reload
          </button>
        </div>
      );
    }
    return this.props.children;
  }
}

export default function App() {
  return (
    <ErrorBoundary>
      <Dashboard />
    </ErrorBoundary>
  );
}
