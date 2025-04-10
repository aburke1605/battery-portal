import { useEffect, useState } from "react";
import axios from "axios";
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Label } from "recharts";
import apiConfig from "../../apiConfig";

interface DataChartProps {
  esp_id: string;
  column: string;
}

interface DataPoint {
  timestamp: string;
  value: number;
}

const formatTimestamp = (timestamp: string) => {
  return new Date(timestamp).toLocaleString("en-GB", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false,
    timeZone: "UTC",
    timeZoneName: "short",
  });
};

const DataChart: React.FC<DataChartProps> = ({ esp_id, column }) => {
  const [data, setData] = useState<DataPoint[]>([]);

  useEffect(() => {
    const fetchData = () => {
      axios.get(`${apiConfig.DB_END_POINT}?esp_id=${esp_id}&column=${column}`)
        .then(response => setData(response.data))
        .catch(error => console.error("Error fetching data:", error));
    };

    fetchData(); // fetch immediately
    const interval = setInterval(fetchData, 5_000); // refresh every five seconds
    return () => clearInterval(interval); // clean-up on unmount
  }, [column]); // re-fetch if column changes(????)

  return (
    <div className="p-4 bg-white shadow rounded-lg">
      <ResponsiveContainer width="100%" height={200}>
        <LineChart data={data} margin={{ top: 10, right: 10, bottom: 15, left: 10 }}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="timestamp" tickFormatter={formatTimestamp} >
            <Label value="Time" offset={-10} position="insideBottom" />
          </XAxis>
          <YAxis>
            <Label value={column} angle={-90} position="insideLeft" style={{ textAnchor: "middle" }} />
          </YAxis>
          <Tooltip labelFormatter={(label) => formatTimestamp(label)} />
          <Line type="monotone" dataKey={column} stroke="#8884d8" dot={{ r: 1 }} />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
};

export default DataChart;
