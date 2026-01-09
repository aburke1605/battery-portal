import React, { createContext, ReactNode, useContext, useState } from "react";
import { Navigate, useNavigate } from "react-router";
import axios from "axios";
import apiConfig from "../apiConfig";

interface UserProps {
  // matches the columns in the users DB table
  id: number;
  first_name: string;
  last_name: string;
  email: string;
  // password: string;
  // active: boolean;
  // confirmed_at: string;
  // fs_uniquifier: string;
  subscribed: boolean;
  subscription_expiry: string;
}

// ===========
// ===========
//   context
//   -------
interface AuthContextType {
  isLoading: boolean;
  user: UserProps | undefined;
  isAuthenticated: boolean;
  register: (
    first_name: string,
    last_name: string,
    email: string,
    password: string,
  ) => Promise<boolean>;
  login: (email: string, password: string) => Promise<boolean>;
  logout: () => Promise<void>;
  fetchUserData: () => Promise<void>;
  getAuthenticationToken: () => string | null; // this one is for ESP32-use only
}
const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const fromAuthenticator = (): AuthContextType => {
  const context = useContext(AuthContext);
  if (!context) throw new Error("fromAuthenticator must be used within ____");
  return context;
};

interface AuthenticationProps {
  children: ReactNode;
  isFromESP32?: boolean;
}
// ============
// ============
//   provider
//   --------
export const AuthenticationProvider: React.FC<AuthenticationProps> = ({
  children,
  isFromESP32 = false,
}) => {
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [user, setUser] = useState<UserProps>();
  const [isAuthenticated, setIsAuthenticated] = useState<boolean>(false);

  const navigate = useNavigate();
  const register = async (
    first_name: string,
    last_name: string,
    email: string,
    password: string,
  ): Promise<boolean> => {
    const response = await axios.post(`${apiConfig.USER_API}/add`, {
      first_name,
      last_name,
      email,
      password,
    });
    if (response.statusText === "CREATED") return login(email, password);
    else return false;
  };
  const login = async (email: string, password: string): Promise<boolean> => {
    const response = await axios.post(`${apiConfig.USER_API}/login`, {
      email,
      password,
    });
    if (response.statusText === "OK") {
      if (response.data.success === true) {
        setIsAuthenticated(true);
        await fetchUserData();
        if (isFromESP32) {
          localStorage.setItem("authenticationToken", response.data.auth_token);
          navigate("/");
        } else {
          navigate("/dashboard");
        }
        return true;
      }
    }
    return false;
  };
  const logout = async (): Promise<void> => {
    setIsAuthenticated(false);
    navigate("/login");
  };

  const fetchUserData = async (): Promise<void> => {
    const response = await axios.get(`${apiConfig.USER_API}/data`);
    if (response.statusText === "OK") setUser(response.data);
  };

  // this one is for ESP32-use only:
  const getAuthenticationToken = () => {
    return localStorage.getItem("authenticationToken");
  };

  const AuthContextValue: AuthContextType = {
    isLoading,
    user,
    isAuthenticated,
    register,
    login,
    logout,
    fetchUserData,
    getAuthenticationToken, // this one is for ESP32-use only
  };
  return (
    <AuthContext.Provider value={AuthContextValue}>
      {!isLoading && children}
    </AuthContext.Provider>
  );
};

// ===========
// ===========
//   -------
//   -------
const AuthenticationRequired: React.FC<AuthenticationProps> = ({
  children,
  isFromESP32 = false,
}) => {
  const { isLoading, isAuthenticated } = fromAuthenticator();

  if (isLoading) return <div>Loading...</div>;
  if (!isAuthenticated)
    return <Navigate to="/login" replace state={{ isFromESP32 }} />;
  return <>{children}</>;
};
export default AuthenticationRequired;
