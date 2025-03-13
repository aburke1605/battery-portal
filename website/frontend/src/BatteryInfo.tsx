import React, { useEffect, useState } from "react";
import { useParams } from "react-router-dom";

interface BatteryData {
  charge: number;
  voltage: number;
  current: number;
  temperature: number;
}

const BatteryInfo: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const [batteryData, setBatteryData] = useState<BatteryData | null>(null);

  useEffect(() => {
    if (!id) return; // Ensure id is defined before proceeding

    const socket = new WebSocket(`wss://${window.location.host}/browser_ws`);

    socket.onopen = () => {
      console.log("WebSocket connection established");
    };

    socket.onmessage = (event) => {
        console.log(event.data);
      try {
        const data = JSON.parse(event.data);
        if (data[id]) {
          setBatteryData({
            charge: data[id].charge,
            voltage: parseFloat(data[id].voltage.toFixed(2)),
            current: parseFloat(data[id].current.toFixed(2)),
            temperature: parseFloat(data[id].temperature.toFixed(2)),
          });
        }
      } catch (error) {
        console.error("Error parsing JSON:", error);
      }
    };

    socket.onerror = (error) => {
      console.error("WebSocket error:", error);
    };

    socket.onclose = () => {
      console.log("WebSocket connection closed");
    };

    return () => {
      socket.close();
    };
  }, [id]);

  return (
    <div style={{ textAlign: "center", padding: "20px" }}>
      <h2>Battery Information</h2>
      {batteryData ? (
        <div>
          <div>
            <span>State of charge: </span>
            <span>{batteryData.charge} %</span>
          </div>
          <div>
            <span>Voltage: </span>
            <span>{batteryData.voltage} V</span>
          </div>
          <div>
            <span>Current: </span>
            <span>{batteryData.current} A</span>
          </div>
          <div>
            <span>Temperature: </span>
            <span>{batteryData.temperature} °C</span>
          </div>
        </div>
      ) : (
        <p>Loading battery data...</p>
      )}
    </div>
  );
};

export default BatteryInfo;
