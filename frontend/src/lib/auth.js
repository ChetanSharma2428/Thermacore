import {
  signInAnonymously,
  signInWithEmailAndPassword,
  createUserWithEmailAndPassword,
  signOut,
  onAuthStateChanged
} from 'firebase/auth';
import { auth, isConfigured } from './firebase';

export async function signInAnonymousUser() {
  if (!isConfigured) {
    console.warn('Firebase is not configured. Running in unauthenticated demo mode.');
    return { uid: 'demo-local-user', isAnonymous: true };
  }
  try {
    const cred = await signInAnonymously(auth);
    return cred.user;
  } catch (error) {
    console.error('Firebase anonymous auth failed:', error);
    throw error;
  }
}

export async function signInWithEmail(email, password) {
  if (!isConfigured) {
    return { uid: 'demo-user', email };
  }
  const cred = await signInWithEmailAndPassword(auth, email, password);
  return cred.user;
}

export async function registerWithEmail(email, password) {
  if (!isConfigured) {
    return { uid: 'demo-user', email };
  }
  const cred = await createUserWithEmailAndPassword(auth, email, password);
  return cred.user;
}

export async function signOutUser() {
  if (!isConfigured) return;
  await signOut(auth);
}

export function onAuthChange(callback) {
  if (!isConfigured) {
    callback({ uid: 'demo-local-user', isAnonymous: true });
    return () => {};
  }
  return onAuthStateChanged(auth, callback);
}
