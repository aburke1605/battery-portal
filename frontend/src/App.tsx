import { createRoot } from "react-dom/client";
import "./index.css";
import "swiper/swiper-bundle.css";
import "simplebar-react/dist/simplebar.min.css";
import "flatpickr/dist/flatpickr.css";
import { AppWrapper } from "./components/common/PageMeta.tsx";
import { ThemeProvider } from "./context/ThemeContext.tsx";
import { HashRouter, Routes, Route } from "react-router";
import SignIn from "./pages/SignIn.tsx";
import Register from "./pages/Register.tsx";
import NotFound from "./pages/NotFound.tsx";
import UserProfiles from "./pages/UserProfiles";
import SystemSettings from "./pages/Settings";
import Blank from "./pages/Blank";
import AppLayout from "./layout/AppLayout";
import { ScrollToTop } from "./components/common/ScrollToTop";
import Home from "./pages/Dashboard";
import ListPage from "./pages/Battery/ListPage.tsx";
import BatteryPage from "./pages/Battery/BatteryPage";
import UserList from "./pages/Users";
import AuthRequire from "./auth/AuthRequire.tsx";
import { AuthProvider } from "./auth/AuthContext.tsx";
import Db from "./pages/Db";
import Visualisation from "./pages/Battery/Visualisation.tsx";
import HomePage from "./home.tsx";
import SubscriptionManagement from "./pages/SubscriptionManagement.tsx";

function App() {
  return (
    <>
      <HashRouter>
        <AuthProvider>
          <ScrollToTop />
          <Routes>
            {/* Home page */}
            <Route path="/" element={<HomePage />} />

            {/* Dashboard Layout */}
            <Route
              element={
                <AuthRequire>
                  <AppLayout />
                </AuthRequire>
              }
            >
              <Route path="/dashboard" element={<Home />} />

              {/* Others Page */}
              <Route path="/profile" element={<UserProfiles />} />
              <Route
                path="/subscription"
                element={<SubscriptionManagement />}
              />
              <Route path="/userlist" element={<UserList />} />
              <Route path="/blank" element={<Blank />} />

              {/* Forms */}
              <Route path="/settings" element={<SystemSettings />} />
              <Route path="/batteries" element={<ListPage />} />
              <Route path="/battery-detail" element={<BatteryPage />} />
              <Route path="/visualisation" element={<Visualisation />} />

              <Route path="/db" element={<Db />} />
            </Route>

            {/* Auth Layout */}
            <Route path="/login" element={<SignIn />} />
            <Route path="/register" element={<Register />} />

            {/* Fallback Route */}
            <Route path="*" element={<NotFound />} />
          </Routes>
        </AuthProvider>
      </HashRouter>
    </>
  );
}

createRoot(document.getElementById("root")!).render(
  <ThemeProvider>
    <AppWrapper>
      <App />
    </AppWrapper>
  </ThemeProvider>,
);
