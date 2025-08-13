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


  const [batteries] = useState<BatteryData[]>(initialBatteries);
  const [alerts] = useState<AlertData[]>(initialAlerts);

  // Get system health status
  const getSystemHealth = () => {
    const criticalCount = batteries.filter(b => b.status === 'critical').length;
    const warningCount = batteries.filter(b => b.status === 'warning').length;
    
    if (criticalCount > 0) return 'Critical';
    if (warningCount > 0) return 'Warning';
    return 'Good';
  };

   // Get system health color
   const getSystemHealthColor = () => {
    const health = getSystemHealth();
    if (health === 'Critical') return 'text-red-600';
    if (health === 'Warning') return 'text-amber-600';
    return 'text-green-600';
  };

  // Get average charge level
  const getAverageChargeLevel = () => {
    const total = batteries.reduce((sum, battery) => sum + battery.charge, 0);
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
    axios.get(apiConfig.DB_ID_END_POINT)
      .then(response => {
        if (response.data != null) {
          setIDs(response.data);
        }
      })
      .catch(err => console.error(err));
  }, []);

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
              <div className="text-sm text-gray-500 mb-1">System Health</div>
              <div className={`text-2xl font-bold ${getSystemHealthColor()}`}>{getSystemHealth()}</div>
            </div>
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
          <h2 className="text-xl font-semibold text-gray-800 mb-4">Live Data</h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-2 gap-20">
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_01</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_01" column="soc" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_02</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_02" column="soc" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_01</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_01" column="temperature" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_02</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_02" column="temperature" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_01</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_01" column="voltage" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_02</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_02" column="voltage" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_01</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_01" column="current" />
              </div>
            </div>
            <div className="bg-gray-50 p-4 rounded-lg">
              <div className="text-sm text-gray-500 mb-1">bms_02</div>
              <div className="h-64 w-full">
                <DataChart esp_id="bms_02" column="current" />
              </div>
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
