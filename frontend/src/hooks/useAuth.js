import { useState, useEffect } from 'react';
import { onAuthChange, signInAnonymousUser, signOutUser } from '../lib/auth';
import { isConfigured } from '../lib/firebase';

export function useAuth() {
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);
  const [authError, setAuthError] = useState(null);

  useEffect(() => {
    const unsubscribe = onAuthChange((currentUser) => {
      setUser(currentUser);
      setLoading(false);
    });

    // Auto sign-in anonymously for prototype / evaluation if not logged in
    if (isConfigured) {
      signInAnonymousUser().catch((err) => {
        setAuthError(err.message || 'Authentication error');
        setLoading(false);
      });
    } else {
      setLoading(false);
    }

    return () => unsubscribe();
  }, []);

  return {
    user,
    loading,
    authError,
    isConfigured,
    signOut: signOutUser
  };
}
