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
import SignIn from "./pages/SignIn.tsx";
import AuthenticationRequired, {
  AuthenticationProvider,
} from "./auth/UserAuthenticator.tsx";

createRoot(document.getElementById("root")!).render(
  <ThemeProvider>
    <AppWrapper>
      <Router>
        <AuthenticationProvider isFromESP32={true}>
          <ScrollToTop />
          <Routes>
            <Route
              index
              path="/"
              element={
                <AuthenticationRequired isFromESP32={true}>
                  <BatteryPage isFromESP32={true} />
                </AuthenticationRequired>
              }
            />
            <Route path="/login" element={<SignIn />} />
          </Routes>
        </AuthenticationProvider>
      </Router>
    </AppWrapper>
  </ThemeProvider>,
);
