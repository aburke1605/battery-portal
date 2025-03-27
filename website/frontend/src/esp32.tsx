import { createRoot } from "react-dom/client";
import "./index.css";
import "swiper/swiper-bundle.css";
import "simplebar-react/dist/simplebar.min.css";
import "flatpickr/dist/flatpickr.css";
import { AppWrapper } from "./components/common/PageMeta.tsx";
import { ThemeProvider } from "./context/ThemeContext.tsx";
import BatteriesPage from "./pages/Battery/DetailPage.tsx";
import { BrowserRouter as Router } from "react-router";
import { ScrollToTop } from "./components/common/ScrollToTop";
import { setupMockWebSocket } from './mock/ws';

setupMockWebSocket();

createRoot(document.getElementById("root")!).render(
    <ThemeProvider>
      <AppWrapper>
        <Router>
        <ScrollToTop />
        <BatteriesPage isFromEsp32={true}/>
        </Router>
      </AppWrapper>
    </ThemeProvider>
);
