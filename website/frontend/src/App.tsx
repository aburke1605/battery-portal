import React from "react";
import { BrowserRouter, Route, Routes } from 'react-router-dom';
import BatteryInfo from "./BatteryInfo";
import BatteriesPage from "./BatteriesPage";

const App: React.FC = () => {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/react" element={<BatteriesPage />} />
        <Route path="/react/battery/:id" element={<BatteryInfo />} />
      </Routes>
    </BrowserRouter>
  );
};

export default App
