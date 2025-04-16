import { createRoot } from "react-dom/client";
import "./index.css";
import "swiper/swiper-bundle.css";
import "simplebar-react/dist/simplebar.min.css";
import "flatpickr/dist/flatpickr.css";
import { AppWrapper } from "./components/common/PageMeta.tsx";
import { ThemeProvider } from "./context/ThemeContext.tsx";
import BatteryPage from "./pages/Battery/BatteryPage.tsx";
import { HashRouter as Router, Routes, Route } from "react-router";
import { ScrollToTop } from "./components/common/ScrollToTop";
import { setupMockWebSocket } from './mock/ws';
import AuthRequire from "./auth/AuthRequire.tsx";
import {AuthProvider} from "./auth/AuthContext.tsx";
import SignIn from "./pages/AuthPages/SignIn";

setupMockWebSocket();

createRoot(document.getElementById("root")!).render(
    <ThemeProvider>
      <AppWrapper>
        <Router>
        <AuthProvider isFromEsp32={true} >
        <ScrollToTop />
        <Routes>
          <Route index path="/" element={
                <AuthRequire>
                  <BatteryPage isFromEsp32={true} />
                </AuthRequire>
                } />
            <Route path="/login" element={<SignIn />} />
          </Routes>
        </AuthProvider>
        </Router>
      </AppWrapper>
    </ThemeProvider>
);
