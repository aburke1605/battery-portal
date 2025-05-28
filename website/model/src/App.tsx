import React from 'react';
import BatteryPack3D, { CellData } from "./BatteryPack";

const App = () => {
  const cells: CellData[] = [
    { label: "cell 1", temperature: 5, position: [-3, 0, -3], charge: 0.8 },
    { label: "cell 2", temperature: 10, position: [-1, 0, -3], charge: 0.9 },
    { label: "cell 3", temperature: 15, position: [1, 0, -3], charge: 1.0 },
    { label: "cell 4", temperature: 20, position: [3, 0, -3], charge: 0.9 },
    { label: "cell 5", temperature: 25, position: [3, 0, -1], charge: 0.8 },
    { label: "cell 6", temperature: 30, position: [1, 0, -1], charge: 0.7 },
    { label: "cell 7", temperature: 35, position: [-1, 0, -1], charge: 0.6 },
    { label: "cell 8", temperature: 40, position: [-3, 0, -1], charge: 0.5 },
    { label: "cell 9", temperature: 45, position: [-3, 0, 1], charge: 0.4 },
    { label: "cell 10", temperature: 50, position: [-1, 0, 1], charge: 0.3 },
    { label: "cell 11", temperature: 55, position: [1, 0, 1], charge: 0.2 },
    { label: "cell 12", temperature: 60, position: [3, 0, 1], charge: 0.1 },
    { label: "cell 13", temperature: 65, position: [3, 0, 3], charge: 0.0 },
    { label: "cell 14", temperature: 70, position: [1, 0, 3], charge: 0.1 },
    { label: "cell 15", temperature: 75, position: [-1, 0, 3], charge: 0.2 },
    { label: "cell 16", temperature: 80, position: [-3, 0, 3], charge: 0.3 },
  ];
  return (
    <div style={{ height: "100vh" }}>
      <BatteryPack3D cells={cells} />
    </div>
  );
};

export default App;