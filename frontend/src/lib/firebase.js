import { initializeApp, getApps, getApp } from 'firebase/app';
import { getAuth } from 'firebase/auth';
import { getDatabase } from 'firebase/database';

// Helper to strip quotes, whitespace, and tabs from environment variables
const cleanEnv = (val) => {
  if (typeof val !== 'string') return '';
  return val.trim().replace(/^["']|["']$/g, '');
};

const rawApiKey = cleanEnv(import.meta.env.VITE_FIREBASE_API_KEY);
const rawDatabaseUrl = cleanEnv(import.meta.env.VITE_FIREBASE_DATABASE_URL).replace(/\/+$/, '');

const firebaseConfig = {
  apiKey: rawApiKey,
  authDomain: cleanEnv(import.meta.env.VITE_FIREBASE_AUTH_DOMAIN),
  databaseURL: rawDatabaseUrl,
  projectId: cleanEnv(import.meta.env.VITE_FIREBASE_PROJECT_ID),
  storageBucket: cleanEnv(import.meta.env.VITE_FIREBASE_STORAGE_BUCKET),
  messagingSenderId: cleanEnv(import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID),
  appId: cleanEnv(import.meta.env.VITE_FIREBASE_APP_ID)
};

// Check if credentials are valid
export const isConfigured = Boolean(
  firebaseConfig.apiKey &&
  firebaseConfig.databaseURL &&
  !firebaseConfig.apiKey.includes('demo') &&
  !firebaseConfig.apiKey.includes('YOUR_') &&
  !firebaseConfig.apiKey.includes('Example')
);

// Safe initialization
let appInstance = null;
let authInstance = null;
let rtdbInstance = null;

try {
  if (isConfigured) {
    appInstance = getApps().length === 0 ? initializeApp(firebaseConfig) : getApp();
    authInstance = getAuth(appInstance);
    rtdbInstance = getDatabase(appInstance);
  }
} catch (err) {
  console.error('Firebase initialization error:', err);
}

export const app = appInstance;
export const auth = authInstance;
export const rtdb = rtdbInstance;

export default appInstance;
