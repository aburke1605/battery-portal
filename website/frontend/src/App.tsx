import React from "react";
import { BrowserRouter, Route, Routes } from 'react-router-dom';
import BatteryInfo from "./BatteryInfo";

const App: React.FC = () => {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/react" element={
          <div style={{ textAlign: "center", padding: "20px" }}>
            <h1>Welcome to My React Page</h1>
            <p>This is the overview of ESP32s.</p>
          </div>
        } />

        <Route path="/react/battery/:id" element={<BatteryInfo />} />
      </Routes>
    </BrowserRouter>
  );
};

export default App
