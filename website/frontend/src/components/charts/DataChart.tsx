import { useEffect, useState } from "react";
import axios from "axios";
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Label } from "recharts";

interface DataPoint {
  timestamp: string;
  value: number;
}

const formatTimestamp = (timestamp: string) => {
  return new Date(timestamp).toLocaleString("en-GB", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false, // Forces 24-hour format
    timeZoneName: "short",
  });
};

const DataChart = () => {
  const [data, setData] = useState<DataPoint[]>([]);

  useEffect(() => {
    axios.get("https://localhost:5000/db/data?esp_id=BMS_03")
      .then(response => setData(response.data))
      .catch(error => console.error("Error fetching data:", error));
  }, []);

  return (
    <div className="p-4 bg-white shadow rounded-lg">
      <ResponsiveContainer width="100%" height={200}>
        <LineChart data={data} margin={{ top: 10, right: 10, bottom: 15, left: 10 }}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="timestamp" tickFormatter={formatTimestamp} >
            <Label value="Time" offset={-10} position="insideBottom" />
          </XAxis>
          <YAxis>
            <Label value="Value" angle={-90} position="insideLeft" style={{ textAnchor: "middle" }} />
          </YAxis>
          <Tooltip labelFormatter={(label) => formatTimestamp(label)} />
          <Line type="monotone" dataKey="value" stroke="#8884d8" />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
};

export default DataChart;
