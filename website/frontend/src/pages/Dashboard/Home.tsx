import{ useState, useEffect } from 'react';
import PageMeta from "../../components/common/PageMeta";
import { BatteryData, AlertData } from '../..//types';
import { initialBatteries, initialAlerts } from '../../mock/mockData';
import { AlertTriangle, Info } from 'lucide-react';
import { formatDateTime, getAlertTypeColor } from '../../utils/helpers';
import DataChart from '../../components/charts/DataChart';
import axios from 'axios';
import apiConfig from '../../apiConfig';

export default function Home() {


  const [batteries, setBatteryData] = useState<BatteryData[]>(initialBatteries);
  useEffect(() => {
    const fetchBatteryData = async () => {
      try {
        const response = await axios.get(`${apiConfig.DB_INFO_API}`);
        setBatteryData(response.data);
      } catch(error) {
        console.error("Error fetching battery data:", error);
      } finally {
        // setLoading(false);
      }
    };

    fetchBatteryData()
    const interval = setInterval(fetchBatteryData, 10000)
    return () => {
      clearInterval(interval)
    }
  }, [])

  const [alerts] = useState<AlertData[]>(initialAlerts);

  // Get average charge level
  const getAverageChargeLevel = () => {
    const total = batteries.reduce((sum, battery) => sum + battery.Q, 0);
    return Math.round(total / batteries.length);
  };

  // Get alert icon based on type
  const getAlertIcon = (type: string) => {
    switch (type) {
      case 'critical': return <AlertTriangle size={20} className="text-red-600" />;
      case 'warning': return <AlertTriangle size={20} className="text-amber-600" />;
      case 'info': return <Info size={20} className="text-blue-600" />;
      default: return <Info size={20} className="text-blue-600" />;
    }
  };

  // get list of esp32 IDs in battery_info table
  const [IDs, setIDs] = useState<[]>([]);
  useEffect(() => {
    const getIDs = async () => {
      try {
        const response = await axios.get(apiConfig.DB_ESP_ID_API);
        if (response.data != null) {
          setIDs(response.data);
        }
      } catch(err) {
        console.error(err);
      }
    }
    getIDs();
  }, []);
  // to set the displayed battery
  const [selectedID, setSelectedID] = useState("");

  // tick-box options for charts
  const chartOptions = ["Q", "H", "V", "V1", "V2", "V3", "V4", "I", "I1", "I2", "I3", "I4", "aT", "cT", "T1", "T2", "T3", "T4"];
  const [selectedCharts, setSelectedCharts] = useState<string[]>([]);
  const toggleOption = (option: string) => {
  setSelectedCharts((prev) =>
    prev.includes(option)
      ? prev.filter((o) => o !== option)
      : [...prev, option]
  );
};

  return (
    <>
      <PageMeta
        title="Dashboard"
        description="Dashboard"
      />
      <div className="space-y-6">
        {/* Overview */}
        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
          <h2 className="text-xl font-semibold text-gray-800 mb-4">System Overview</h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6">
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">Total Batteries</div>
              <div className="text-2xl font-bold text-gray-800">{batteries.length}</div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">Average Charge</div>
              <div className="text-2xl font-bold text-blue-600">{getAverageChargeLevel()}%</div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">Active Alerts</div>
              <div className="text-2xl font-bold text-gray-800">
                {alerts.filter(a => !a.isRead).length}
              </div>
            </div>
          </div>
        </div>

        {/* Live Data */}
        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
          <div className="flex items-center justify-between mb-4">
            <h2 className="text-xl font-semibold text-gray-800">Telemetry data</h2>

            {/* tick box */}
            <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-4 ml-6 mr-6">
              <div className="flex flex-wrap gap-x-4 gap-y-2">
                {chartOptions.map((option) => (
                  <label key={option} className="flex items-center space-x-2">
                    <input
                      type="checkbox"
                      checked={selectedCharts.includes(option)}
                      onChange={() => toggleOption(option)}
                      className="h-4 w-4 text-indigo-600 border-gray-300 rounded focus:ring-indigo-500"
                    />
                    <span className="text-gray-700">{option}</span>
                  </label>
                ))}
              </div>
            </div>

            {/* drop down menu */}
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1 text-right">
                Select a device
              </label>
              <select
                value={selectedID}
                onChange={(e) => setSelectedID(e.target.value)}
                className="rounded-lg border border-gray-300 bg-white px-3 py-2 text-gray-700 shadow-sm focus:border-indigo-500 focus:ring focus:ring-indigo-200 focus:ring-opacity-50"
              >
                <option value="" disabled className="text-center">
                  -- Choose an ID --
                </option>
                {IDs.map((id, idx) => (
                  <option key={idx} value={id} className="text-center">
                    {id}
                  </option>
                ))}
              </select>
            </div>
          </div>

          {/* chart(s) */}
          <div className="bg-gray-50 rounded-lg p-6 mb-4">
            <div className="mx-auto grid grid-cols-1 gap-4 sm:grid-cols-2 max-w-4xl mb-4">
              {selectedCharts.map((option) => (
                <div className="w-full aspect-[5/3]">
                  <DataChart esp_id={selectedID} column={option} />
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Recent Alerts */}
        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
          <div className="flex justify-between items-center mb-4">
            <h2 className="text-xl font-semibold text-gray-800">Recent Alerts</h2>
            <button 
              className="text-blue-600 hover:text-blue-800 text-sm font-medium"
             
            >
              View All
            </button>
          </div>
            <div className="space-y-4">
              {alerts.slice(0, 3).map(alert => (
                <div 
                  key={alert.id} 
                  className={`p-4 rounded-lg ${alert.isRead ? 'bg-white border border-gray-200' : 'bg-blue-50 border border-blue-200'}`}
                >
                  <div className="flex items-start">
                    <div className="flex-shrink-0 mt-1">
                      <div className={`h-8 w-8 rounded-full flex items-center justify-center ${
                        alert.type === 'critical' ? 'bg-red-100' : 
                        alert.type === 'warning' ? 'bg-amber-100' : 'bg-blue-100'
                      }`}>
                        {getAlertIcon(alert.type)}
                      </div>
                    </div>
                    <div className="ml-3 flex-1">
                      <div className="flex items-center justify-between">
                        <h3 className="text-sm font-medium text-gray-900">{alert.message}</h3>
                        <span className={`px-2 py-0.5 text-xs rounded-full ${getAlertTypeColor(alert.type)}`}>
                          {alert.type.charAt(0).toUpperCase() + alert.type.slice(1)}
                        </span>
                      </div>
                      <p className="mt-1 text-xs text-gray-600">
                        Battery: <span className="font-medium">{alert.batteryName}</span>
                      </p>
                      <div className="mt-1 text-xs text-gray-500">
                        {formatDateTime(alert.timestamp)}
                      </div>
                    </div>
                  </div>
                </div>
              ))}
            </div>
        </div>
      </div>
    </>
  );
}
