import { HashRouter as Router, Routes, Route } from "react-router";
import SignIn from "./pages/AuthPages/SignIn";
import NotFound from "./pages/OtherPage/NotFound";
import UserProfiles from "./pages/UserProfiles";
import SystemSettings from "./pages/Settings/SystemSettings";
import Blank from "./pages/Blank";
import AppLayout from "./layout/AppLayout";
import { ScrollToTop } from "./components/common/ScrollToTop";
import Home from "./pages/Dashboard/Home";
import ListPage from "./pages/Battery/ListPage.tsx";
import BatteryPage from "./pages/Battery/BatteryPage";
import UserList from "./pages/User/list";
import AuthRequire from "./auth/AuthRequire.tsx";
import {AuthProvider} from "./auth/AuthContext.tsx";
import Db from "./pages/Db";


export default function App() {

  return (
    <>
      <Router>
        <AuthProvider>
        <ScrollToTop />
        <Routes>
          {/* Dashboard Layout */}
          <Route element={<AppLayout />}>
            <Route index path="/" element={
              <AuthRequire>
                <Home />
              </AuthRequire>
              } />

            {/* Others Page */}
            <Route path="/profile" element={<UserProfiles />} />
            <Route path="/userlist" element={<UserList />} />
            <Route path="/blank" element={<Blank />} />

            {/* Forms */}
            <Route path="/settings" element={<SystemSettings />} />
            <Route path="/batteries" element={<ListPage />} />
            <Route path="/battery-detail" element={<BatteryPage />} />

            <Route path="/db" element={<Db />} />
          </Route>

          {/* Auth Layout */}
          <Route path="/login" element={<SignIn />} />

          

          {/* Fallback Route */}
          <Route path="*" element={<NotFound />} />

        </Routes>
        </AuthProvider>
      </Router>
    </>
  );
}
