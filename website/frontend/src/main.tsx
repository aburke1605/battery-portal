import { createRoot } from "react-dom/client";
import "./index.css";
import "swiper/swiper-bundle.css";
import "simplebar-react/dist/simplebar.min.css";
import "flatpickr/dist/flatpickr.css";
import App from "./App.tsx";
import { AppWrapper } from "./components/common/PageMeta.tsx";
import { ThemeProvider } from "./context/ThemeContext.tsx";
import { setupMockWebSocket } from './mock/ws';
import LoginForm from "./esp32.tsx";
import { useState } from "react";

function Test() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);

  const handleLogin = async (username: string, password: string) => {
    try {
      const res = await fetch("/api/users/login", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include", // important for cookie-based auth
        body: JSON.stringify({ username, password }),
      });

      if (res.ok) setIsLoggedIn(true);
      return res.ok;
    } catch (err) {
      console.error("Login failed:", err);
      return false;
    }
  };

  return isLoggedIn ? (
    <App />
  ) : (
    <LoginForm onLogin={handleLogin} />
  );
}

setupMockWebSocket();
createRoot(document.getElementById("root")!).render(
    <ThemeProvider>
      <AppWrapper>
        <Test />
      </AppWrapper>
    </ThemeProvider>
);
