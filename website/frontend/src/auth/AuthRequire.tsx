// components/AuthRequire.tsx
import React, { ReactNode } from 'react';
import { Navigate } from 'react-router-dom';
import { useAuth } from './AuthContext';

interface AuthRequireProps {
  children: ReactNode;
  isFromESP32?: boolean;
}

const AuthRequire: React.FC<AuthRequireProps> = ({ children, isFromESP32 = false }) => {
  const { isAuthenticated, loading } = useAuth();

  if (loading) {
    return <div>Loading...</div>;
  }

  if (!isAuthenticated) {
    return <Navigate to="/login" replace state={{ isFromESP32 }}/>;
  }

  return <>{children}</>;
};

export default AuthRequire;
