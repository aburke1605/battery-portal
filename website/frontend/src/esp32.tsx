import { useState } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import "swiper/swiper-bundle.css";
import "simplebar-react/dist/simplebar.min.css";
import "flatpickr/dist/flatpickr.css";
import { AppWrapper } from "./components/common/PageMeta.tsx";
import { ThemeProvider } from "./context/ThemeContext.tsx";
import BatteryPage from "./pages/Battery/BatteryPage.tsx";
import { BrowserRouter as Router } from "react-router";
import { ScrollToTop } from "./components/common/ScrollToTop";
import { setupMockWebSocket } from './mock/ws';
import apiConfig from "./apiConfig.tsx";

function LoginForm({ onLogin }: { onLogin: (u: string, p: string) => Promise<boolean> }) {
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    const success = await onLogin(username, password);
    if (!success) setError("Invalid credentials");
  };

  return (
    <form onSubmit={handleSubmit} className="flex flex-col items-center mt-20 gap-4">
      <input
        className="border p-2 rounded"
        type="text"
        placeholder="Username"
        value={username}
        onChange={(e) => setUsername(e.target.value)}
      />
      <input
        className="border p-2 rounded"
        type="password"
        placeholder="Password"
        value={password}
        onChange={(e) => setPassword(e.target.value)}
      />
      <button type="submit" className="bg-blue-500 text-white px-4 py-2 rounded">
        Login
      </button>
      {error && <p className="text-red-500">{error}</p>}
    </form>
  );
}

function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);

  const handleLogin = async (username: string, password: string) => {
    const success = username === apiConfig.ADMIN_USERNAME && password === apiConfig.ADMIN_PASSWORD;

    if (success) setIsLoggedIn(true);

    return success;
  };

  return isLoggedIn ? (
    <BatteryPage isFromEsp32={true} />
  ) : (
    <LoginForm onLogin={handleLogin} />
  );
}

setupMockWebSocket();

createRoot(document.getElementById("root")!).render(
    <ThemeProvider>
      <AppWrapper>
        <Router>
        <ScrollToTop />
        <App />
        </Router>
      </AppWrapper>
    </ThemeProvider>
);
