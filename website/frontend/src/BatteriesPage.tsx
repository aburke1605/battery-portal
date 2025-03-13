import React, { useEffect, useState } from "react";
import { Link, useParams } from "react-router-dom";
import BatteryInfo from "./BatteryInfo";

const BatteriesPage: React.FC = () => {
  const [batteryIds, setBatteryIds] = useState<string[]>([]);
  const { id } = useParams<{ id?: string }>();

  useEffect(() => {
    // Simulate fetching available battery IDs (replace with real data source)
    setBatteryIds(["BMS_01", "BMS_05"]);
  }, []);

  return (
    <div style={{ textAlign: "center", padding: "20px" }}>
      <h1>Battery Overview</h1>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(150px, 1fr))", gap: "10px", padding: "20px" }}>
        {batteryIds.map((batteryId) => (
          <Link
            key={batteryId}
            to={`/react/battery/${batteryId}`}
            style={{ textDecoration: "none", color: "black" }}
          >
            <div style={{ border: "1px solid black", padding: "10px", cursor: "pointer" }}>
              {batteryId}
            </div>
          </Link>
        ))}
      </div>
      {id && <BatteryInfo />}
    </div>
  );
};

export default BatteriesPage;
