import { HashRouter as Router, Routes, Route } from "react-router";
import SignIn from "./pages/AuthPages/SignIn";
import SignUp from "./pages/AuthPages/SignUp";
import NotFound from "./pages/OtherPage/NotFound";
import UserProfiles from "./pages/UserProfiles";
import SystemSettings from "./pages/Settings/SystemSettings";
import Blank from "./pages/Blank";
import AppLayout from "./layout/AppLayout";
import { ScrollToTop } from "./components/common/ScrollToTop";
import Home from "./pages/Dashboard/Home";
import ListPage from "./pages/Battery/ListPage";
import BatteryPage from "./pages/Battery/BatteryPage";
import UserList from "./pages/User/list";

export default function App() {

  return (
    <>
      <Router>
        <ScrollToTop />
        <Routes>
          {/* Dashboard Layout */}
          <Route element={<AppLayout />}>
            <Route index path="/" element={<Home />} />

            {/* Others Page */}
            <Route path="/profile" element={<UserProfiles />} />
            <Route path="/userlist" element={<UserList />} />
            <Route path="/blank" element={<Blank />} />

            {/* Forms */}
            <Route path="/settings" element={<SystemSettings />} />
            <Route path="/batteries" element={<ListPage />} />
            <Route path="/battery-detail" element={<BatteryPage />} />

          </Route>

          {/* Auth Layout */}
          <Route path="/signin" element={<SignIn />} />
          <Route path="/signup" element={<SignUp />} />

          {/* Fallback Route */}
          <Route path="*" element={<NotFound />} />
        </Routes>
      </Router>
    </>
  );
}
