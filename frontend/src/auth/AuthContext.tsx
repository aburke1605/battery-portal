// context/AuthContext.tsx
import React, {
  createContext,
  useContext,
  useEffect,
  useState,
  ReactNode,
} from "react";
import { useNavigate } from "react-router-dom";
import apiConfig from "../apiConfig";

interface User {
  email: string;
  [key: string]: any;
}

interface AuthContextType {
  user: User | null;
  isAuthenticated: boolean;
  loading: boolean;
  logout: () => Promise<void>;
  login: (
    email: string,
    password: string,
    isFromESP32?: boolean,
  ) => Promise<boolean>;
  register: (
    firstName: string,
    familyName: string,
    email: string,
    password: string,
  ) => Promise<boolean>;
  getAuthToken: () => string | null;
  refreshUser: () => Promise<void>;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const useAuth = (): AuthContextType => {
  const context = useContext(AuthContext);
  if (!context) {
    throw new Error("useAuth must be used within an AuthProvider");
  }
  return context;
};

interface AuthProviderProps {
  children: ReactNode;
  isFromESP32?: boolean;
}

export const AuthProvider: React.FC<AuthProviderProps> = ({
  children,
  isFromESP32 = false,
}) => {
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const navigate = useNavigate();

  const checkAuthStatus = async () => {
    try {
      let url = `${apiConfig.USER_API}/check-auth`;
      if (isFromESP32) {
        const token = getAuthToken();
        url += "?auth_token=" + token;
      }

      const response = await fetch(url, {
        method: "GET",
        credentials: "include",
      });

      if (response.ok) {
        const data = await response.json();
        setUser(data);
      } else {
        setUser(null);
      }
    } catch (error) {
      console.error("Auth check failed:", error);
      setUser(null);
    }
  };

  useEffect(() => {
    checkAuthStatus().finally(() => setLoading(false));
  }, []);

  const logout = async () => {
    try {
      await fetch(`${apiConfig.USER_API}/logout`, {
        method: "POST",
        credentials: "include",
      });
    } catch (err) {
      console.error("Logout failed:", err);
    } finally {
      setUser(null);
      navigate("/login");
    }
  };

  const login = async (
    email: string,
    password: string,
    isFromESP32?: boolean,
  ): Promise<boolean> => {
    try {
      const res = await fetch(`${apiConfig.USER_API}/login`, {
        method: "POST",
        credentials: "include",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ email, password }),
      });

      if (res.ok) {
        const data = await res.json();
        setUser(data);
        setAuthToken(data.auth_token);
        isFromESP32 ? navigate("/") : navigate("/dashboard");
        return true;
      } else {
        return false;
      }
    } catch (err) {
      console.error("Login failed:", err);
      return false;
    }
  };

  const register = async (
    firstName: string,
    familyName: string,
    email: string,
    password: string,
  ): Promise<boolean> => {
    try {
      const res = await fetch(`${apiConfig.USER_API}/add`, {
        method: "POST",
        credentials: "include",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          first_name: firstName,
          last_name: familyName,
          email,
          password,
        }),
      });

      if (res.ok) {
        return login(email, password, false);
      } else {
        return false;
      }
    } catch (err) {
      console.error("Registration failed:", err);
      return false;
    }
  };

  const setAuthToken = (token: string) => {
    if (token) {
      localStorage.setItem("auth_token", token);
    } else {
      localStorage.removeItem("auth_token");
    }
  };

  const getAuthToken = () => {
    return localStorage.getItem("auth_token");
  };

  const contextValue: AuthContextType = {
    user,
    isAuthenticated: !!user,
    loading,
    logout,
    login,
    register,
    getAuthToken,
    refreshUser: checkAuthStatus,
  };

  return (
    <AuthContext.Provider value={contextValue}>
      {!loading && children}
    </AuthContext.Provider>
  );
};
