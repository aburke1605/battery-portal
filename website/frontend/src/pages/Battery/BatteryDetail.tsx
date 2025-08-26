import React, {useEffect, useState} from 'react';
import { BatteryDataNew } from '../../types';
import { 
  Share2, 
  Download, 
  Printer, 
  BatteryLow, 
  BatteryMedium, 
  BatteryFull, 
  Zap, 
  ThermometerSun, 
  Info, 
  RefreshCw, 
  LockKeyholeOpen,
  Wifi, 
  // Power, 
  BrainCircuit,
  FileText, 
  History, 
  BarChart3, 
  AlertTriangle, 
  CheckCircle,
  ArrowLeft,
  Sliders,
  Users,
  Bell,
  Shield,
  Battery,
  PenTool as Tool, Clock,
} from 'lucide-react';
import { getStatusColor } from '../../utils/helpers';
import { useNavigate } from 'react-router-dom';

interface BatteryDetailProps {
  battery: BatteryDataNew;
  // onToggleCharging: (batteryId: string) => void;
  voltageThreshold: number;
  sendBatteryUpdate: (updatedValues: Partial<BatteryDataNew>) => void;
  sendWiFiConnect: (username: string, password: string, eduroam: boolean) => void;
  sendUnseal: () => void;
  sendReset: () => void;
}

const BatteryDetail: React.FC<BatteryDetailProps> = ({ 
  battery, 
  voltageThreshold,
  sendBatteryUpdate, // receive function from BatteryPage
  sendWiFiConnect,
  sendUnseal,
  sendReset,
}) => {
  
  const [activeTab, setActiveTab] = useState('overview');

  const [isEditing, setIsEditing] = useState(false);
  const [hasChanges, setHasChanges] = useState(false);
  const [hasWiFiChanges, setHasWiFiChanges] = useState(false);

  const [ssid, setSSID] = useState("");
  const [password, setPassword] = useState("");
  const [eduroam_username, setEduroamUsername] = useState("");
  const [eduroam_password, setEduroamPassword] = useState("");

  // range
  const OTC_threshold_min = 450;
  const OTC_threshold_max = 650;
  // initialise
  const [values, setValues] = useState<Partial<BatteryDataNew>>({
    esp_id: battery.esp_id,
    OTC_threshold: battery.OTC_threshold
  });
  // websocket update
  useEffect(() => {
    if (!isEditing) {
      setValues({
        esp_id: battery.esp_id,
        OTC_threshold: battery.OTC_threshold
      });
      setHasChanges(false);
    }
  }, [battery.esp_id, battery.OTC_threshold, isEditing]);
  // slider update
  const handleSliderChange = (key: string) => (e: React.ChangeEvent<HTMLInputElement>) => {
    const { value, type } = e.target;
    const newValue = type === "number" || type === "range" ? Number(value) : value; // Convert numbers only for sliders
    setValues((prevValues) => {
      const updatedValues = { ...prevValues, [key]: newValue };
      // check if any slider has moved
      const hasAnyChange = (Object.keys(values) as Array<keyof BatteryDataNew>).some((k) => values[k] !== battery[k as keyof BatteryDataNew]);
      setHasChanges(hasAnyChange);
      setIsEditing(true);
      return updatedValues;
    });
  };
  // send websocket-update message
  const handleSubmit = () => {
    sendBatteryUpdate(values);
    setIsEditing(false);
    setHasChanges(false);
  }
  // reset sliders
  const handleReset = () => {
    setValues({
      OTC_threshold: battery.OTC_threshold
    });
    setIsEditing(false);
    setHasChanges(false);
  };
  // router entry
  const handleTextChange = (setter: React.Dispatch<React.SetStateAction<string>>) =>
    (e: React.ChangeEvent<HTMLInputElement>) => {
      setter(e.target.value);
      setHasWiFiChanges(true);
  };
  // send websocket-connect message
  const handleConnect = () => {
    if (ssid == "" && password == "")
      sendWiFiConnect(eduroam_username, eduroam_password, true);
    else if (eduroam_username == "" && eduroam_password == "")
      sendWiFiConnect(ssid, password, false);
    setSSID("");
    setPassword("");
    setEduroamUsername("");
    setEduroamPassword("");
    setHasWiFiChanges(false);
  }

  const renderTabContent = () => {
    switch (activeTab) {
      case 'performance':
        return (
          <div className="space-y-6">
            <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
              <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                <h3 className="text-lg font-medium text-gray-900 flex items-center">
                  <Battery size={20} className="mr-2" /> Performance Metrics
                </h3>
              </div>
              <div className="p-6">
                <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Efficiency Rate</h4>
                    <div className="text-2xl font-semibold text-gray-900">94%</div>
                    <p className="mt-1 text-sm text-gray-500">Above average performance</p>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Power Output</h4>
                    <div className="text-2xl font-semibold text-gray-900">2.4 kW</div>
                    <p className="mt-1 text-sm text-gray-500">Nominal output</p>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Temperature Variance</h4>
                    <div className="text-2xl font-semibold text-gray-900">±2.5°C</div>
                    <p className="mt-1 text-sm text-gray-500">Within normal range</p>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Response Time</h4>
                    <div className="text-2xl font-semibold text-gray-900">50ms</div>
                    <p className="mt-1 text-sm text-gray-500">Optimal performance</p>
                  </div>
                </div>
              </div>
            </div>

            <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
              <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                <h3 className="text-lg font-medium text-gray-900 flex items-center">
                  <Shield size={20} className="mr-2" /> Safety Parameters
                </h3>
              </div>
              <div className="p-6">
                <div className="space-y-4">
                  <div>
                    <div className="flex justify-between mb-1">
                      <span className="text-sm font-medium text-gray-500">Temperature Protection</span>
                      <span className="text-sm font-medium text-green-600">Active</span>
                    </div>
                    <div className="w-full bg-gray-200 rounded-full h-2">
                      <div className="bg-green-500 h-2 rounded-full" style={{ width: '100%' }}></div>
                    </div>
                  </div>
                  <div>
                    <div className="flex justify-between mb-1">
                      <span className="text-sm font-medium text-gray-500">Overcharge Protection</span>
                      <span className="text-sm font-medium text-green-600">Active</span>
                    </div>
                    <div className="w-full bg-gray-200 rounded-full h-2">
                      <div className="bg-green-500 h-2 rounded-full" style={{ width: '100%' }}></div>
                    </div>
                  </div>
                  <div>
                    <div className="flex justify-between mb-1">
                      <span className="text-sm font-medium text-gray-500">Short Circuit Protection</span>
                      <span className="text-sm font-medium text-green-600">Active</span>
                    </div>
                    <div className="w-full bg-gray-200 rounded-full h-2">
                      <div className="bg-green-500 h-2 rounded-full" style={{ width: '100%' }}></div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        );

      case 'history':
        return (
          <div className="space-y-6">
            <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
              <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                <h3 className="text-lg font-medium text-gray-900 flex items-center">
                  <Clock size={20} className="mr-2" /> Usage History
                </h3>
              </div>
              <div className="p-6">
                <div className="space-y-6">
                  {[
                    {
                      date: '2025-04-02',
                      event: 'Charge Cycle Completed',
                      details: 'Full charge cycle completed in 2.5 hours',
                      type: 'success'
                    },
                    {
                      date: '2025-04-01',
                      event: 'Temperature Warning',
                      details: 'Temperature exceeded 40°C for 15 minutes',
                      type: 'warning'
                    },
                    {
                      date: '2025-03-31',
                      event: 'Maintenance Check',
                      details: 'Routine maintenance check completed',
                      type: 'info'
                    },
                    {
                      date: '2025-03-30',
                      event: 'Performance Test',
                      details: 'Capacity test completed - 95% efficiency',
                      type: 'success'
                    }
                  ].map((event, index) => (
                    <div key={index} className="flex items-start">
                      <div className="flex-shrink-0">
                        <div className={`h-8 w-8 rounded-full flex items-center justify-center ${
                          event.type === 'success' ? 'bg-green-100' :
                          event.type === 'warning' ? 'bg-amber-100' :
                          'bg-blue-100'
                        }`}>
                          {event.type === 'success' ? (
                            <CheckCircle size={16} className="text-green-600" />
                          ) : event.type === 'warning' ? (
                            <AlertTriangle size={16} className="text-amber-600" />
                          ) : (
                            <Info size={16} className="text-blue-600" />
                          )}
                        </div>
                      </div>
                      <div className="ml-3">
                        <p className="text-sm font-medium text-gray-900">{event.event}</p>
                        <p className="text-sm text-gray-500">{event.details}</p>
                        <p className="text-xs text-gray-400 mt-1">{event.date}</p>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          </div>
        );

      case 'maintenance':
        return (
          <div className="space-y-6">
            <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
              <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                <h3 className="text-lg font-medium text-gray-900 flex items-center">
                  <Tool size={20} className="mr-2" /> Maintenance Schedule
                </h3>
              </div>
              <div className="p-6">
                <div className="space-y-6">
                  <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Next Maintenance Due</h4>
                      <p className="text-2xl font-semibold text-gray-900">May 15, 2025</p>
                      <p className="mt-1 text-sm text-gray-500">Regular inspection and testing</p>
                    </div>
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Last Maintenance</h4>
                      <p className="text-2xl font-semibold text-gray-900">March 15, 2025</p>
                      <p className="mt-1 text-sm text-gray-500">Completed by John Smith</p>
                    </div>
                  </div>

                  <div className="mt-6">
                    <h4 className="text-sm font-medium text-gray-500 mb-4">Upcoming Tasks</h4>
                    <div className="space-y-4">
                      {[
                        {
                          task: 'Cell Balancing Check',
                          date: '2025-05-15',
                          status: 'pending',
                          priority: 'high'
                        },
                        {
                          task: 'Thermal Management Inspection',
                          date: '2025-05-15',
                          status: 'pending',
                          priority: 'medium'
                        },
                        {
                          task: 'Connection Testing',
                          date: '2025-05-15',
                          status: 'pending',
                          priority: 'low'
                        }
                      ].map((task, index) => (
                        <div key={index} className="flex items-center justify-between p-4 bg-gray-50 rounded-lg">
                          <div>
                            <h5 className="text-sm font-medium text-gray-900">{task.task}</h5>
                            <p className="text-sm text-gray-500">Due: {task.date}</p>
                          </div>
                          <span className={`px-3 py-1 rounded-full text-xs font-medium ${
                            task.priority === 'high' ? 'bg-red-100 text-red-800' :
                            task.priority === 'medium' ? 'bg-amber-100 text-amber-800' :
                            'bg-green-100 text-green-800'
                          }`}>
                            {task.priority.charAt(0).toUpperCase() + task.priority.slice(1)} Priority
                          </span>
                        </div>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        );

      case 'settings':
        return (
          <div className="space-y-6">
            <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
              <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                <h3 className="text-lg font-medium text-gray-900 flex items-center">
                  <Sliders size={20} className="mr-2" /> Battery Settings
                </h3>
              </div>
              <div className="p-6">
                <div className="space-y-6">
                  <div>
                    <h4 className="text-sm font-medium text-gray-900 mb-4">Charging Parameters</h4>
                    <div className="space-y-4">
                      <div>
                        <label className="block text-sm font-medium text-gray-700">Device name:</label>
                        <input
                          type="text"
                          className="border p-1 w-full"
                          placeholder={battery.esp_id}
                          onChange={handleSliderChange("new_esp_id")}
                        />

                        <label className="block text-sm font-medium text-gray-700">OCT threshold: {values.OTC_threshold} [0.1 °C]</label>
                        <input
                          type="range"
                          className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer mt-2"
                          min={OTC_threshold_min}
                          max={OTC_threshold_max}
                          value={values.OTC_threshold}
                          onChange={handleSliderChange("OTC_threshold")}
                        />
                        <div className="flex justify-between text-xs text-gray-500 mt-1">
                          <span>{OTC_threshold_min} [0.1 °C]</span>
                          <span>{OTC_threshold_max} [0.1 °C]</span>
                        </div>

                        {hasChanges && (
                          <div className="flex gap-2 mt-2">
                            {battery.live_websocket ? (
                              <>
                                <button onClick={handleSubmit} className="p-2 bg-blue-500 text-white rounded">
                                  Submit Updates
                                </button>
                                <button onClick={handleReset} className="p-2 bg-gray-500 text-white rounded">
                                  Reset
                                </button>
                              </>
                            ) : (
                              <button className="p-2 bg-gray-500 text-white rounded">
                                OFFLINE
                              </button>
                            )}
                          </div>
                        )}
                      </div>
                    </div>
                  </div>

                  <div className="pt-6">
                    <h4 className="text-sm font-medium text-gray-900 mb-4">Alert Settings</h4>
                    <div className="space-y-4">
                      <div className="flex items-center justify-between">
                        <div className="flex items-center">
                          <Bell size={16} className="text-gray-400 mr-2" />
                          <span className="text-sm text-gray-700">Temperature Alerts</span>
                        </div>
                        <label className="relative inline-flex items-center cursor-pointer">
                          <input type="checkbox" className="sr-only peer" checked />
                          <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
                        </label>
                      </div>
                      <div className="flex items-center justify-between">
                        <div className="flex items-center">
                          <Bell size={16} className="text-gray-400 mr-2" />
                          <span className="text-sm text-gray-700">Voltage Alerts</span>
                        </div>
                        <label className="relative inline-flex items-center cursor-pointer">
                          <input type="checkbox" className="sr-only peer" checked />
                          <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
                        </label>
                      </div>
                    </div>
                  </div>

                  <div className="pt-6">
                    <h4 className="text-sm font-medium text-gray-900 mb-4">Access Control</h4>
                    <div className="space-y-4">
                      <div className="flex items-center justify-between">
                        <div className="flex items-center">
                          <Users size={16} className="text-gray-400 mr-2" />
                          <span className="text-sm text-gray-700">Technician Access</span>
                        </div>
                        <label className="relative inline-flex items-center cursor-pointer">
                          <input type="checkbox" className="sr-only peer" checked />
                          <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
                        </label>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        );

      case 'wifi':
        return (
          <div className="space-y-6">
            <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
              <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                <h3 className="text-lg font-medium text-gray-900 flex items-center">
                  <Sliders size={20} className="mr-2" /> Wi-Fi Settings
                </h3>
              </div>
              <div className="p-6">
                <div className="space-y-6">
                  <div>
                    <h4 className="text-sm font-medium text-gray-900 mb-4">Router Information</h4>
                    <div className="space-y-4">
                      <div>
                        <label className="block text-sm font-medium text-gray-700">SSID:</label>
                        <input
                          type="text"
                          className="border p-1 w-full"
                          value={ssid}
                          name="ssid"
                          id="ssid"
                          required
                          onChange={handleTextChange(setSSID)}
                        />

                        <label className="block text-sm font-medium text-gray-700">Password:</label>
                        <input
                          type="password"
                          className="border p-1 w-full"
                          value={password}
                          name="password"
                          id="password"
                          required
                          onChange={handleTextChange(setPassword)}
                        />
                      </div>
                    </div>
                  </div>

                  <div>
                    <h4 className="text-sm font-medium text-gray-900 mb-4">Eduroam Information</h4>
                    <div className="space-y-4">
                      <div>
                        <label className="block text-sm font-medium text-gray-700">Username:</label>
                        <input
                          type="text"
                          className="border p-1 w-full"
                          value={eduroam_username}
                          name="eduroam_username"
                          id="eduroam_username"
                          required
                          onChange={handleTextChange(setEduroamUsername)}
                        />

                        <label className="block text-sm font-medium text-gray-700">Password:</label>
                        <input
                          type="password"
                          className="border p-1 w-full"
                          value={eduroam_password}
                          name="eduroam_password"
                          id="eduroam_password"
                          required
                          onChange={handleTextChange(setEduroamPassword)}
                        />

                        {hasWiFiChanges && (
                          <div className="flex gap-2 mt-2">
                            {battery.live_websocket ? (
                              <button onClick={handleConnect} className="p-2 bg-blue-500 text-white rounded">
                                Connect
                              </button>
                            ) : (
                              <button className="p-2 bg-gray-500 text-white rounded">
                                OFFLINE
                              </button>
                            )}
                          </div>
                        )}
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        );

      default: // Overview tab
        return (
          <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
            {/* Left Column - Detailed Stats */}
            <div className="lg:col-span-2 space-y-6">
              <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
                <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                  <h3 className="text-lg font-medium leading-6 text-gray-900 flex items-center">
                    <BarChart3 size={20} className="mr-2" /> Performance Metrics
                  </h3>
                </div>
                <div className="px-4 py-5 sm:p-6">
                  <div className="grid grid-cols-1 sm:grid-cols-2 gap-6">
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Voltage</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.V} V</p>
                      <div className="mt-1 text-sm text-gray-500">
                        {battery.V < voltageThreshold ? (
                          <span className="text-amber-600">Below threshold ({voltageThreshold}V)</span>
                        ) : (
                          <span className="text-green-600">Normal operating range</span>
                        )}
                      </div>
                    </div>
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Current</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.I} A</p>
                      <div className="mt-1 text-sm text-gray-500">
                        {/* {battery.isCharging ? (
                          <span className="text-blue-600">Charging current</span>
                        ) : (
                          <span>Discharge current</span>
                        )} */}
                      </div>
                    </div>
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Cell 1</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.I} A </p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.iT} °C</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.V} V</p>
                    </div>
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Cell 2</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.I} A</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.iT} °C</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.V} V</p>
                    </div>
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Cell 3</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.I} A</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.iT} °C</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.V} V</p>
                    </div>
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Cell 4</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.I} A</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.iT} °C</p>
                      <p className="text-2xl font-semibold text-gray-900">{battery.V} V</p>
                    </div>
                    {/* <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Capacity</h4>
                      <p className="text-2xl font-semibold text-gray-900">{battery.capacity} kWh</p>
                      <div className="mt-1 text-sm text-gray-500">
                        Nominal capacity
                      </div>
                    </div> */}
                    <div>
                      <h4 className="text-sm font-medium text-gray-500 mb-2">Last Maintenance</h4>
                      {/* <p className="text-2xl font-semibold text-gray-900">{battery.lastMaintenance}</p> */}
                      <div className="mt-1 text-sm text-gray-500">
                        {/* {new Date(battery.lastMaintenance) < new Date(Date.now() - 90 * 24 * 60 * 60 * 1000) ? (
                          <span className="text-amber-600">Maintenance due</span>
                        ) : (
                          <span className="text-green-600">Up to date</span>
                        )} */}
                      </div>
                    </div>
                  </div>
                </div>
              </div>

              <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
                <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                  <h3 className="text-lg font-medium leading-6 text-gray-900 flex items-center">
                    <History size={20} className="mr-2" /> Recent Activity
                  </h3>
                </div>
                <div className="px-4 py-5 sm:p-6">
                  <ul className="space-y-4">
                    <li className="flex items-start">
                      <div className="flex-shrink-0">
                        <div className="h-8 w-8 rounded-full bg-blue-100 flex items-center justify-center">
                          <RefreshCw size={16} className="text-blue-600" />
                        </div>
                      </div>
                      <div className="ml-3">
                        <p className="text-sm font-medium text-gray-900">Charge cycle completed</p>
                        <p className="text-sm text-gray-500">Yesterday at 2:15 PM</p>
                      </div>
                    </li>
                    <li className="flex items-start">
                      <div className="flex-shrink-0">
                        <div className="h-8 w-8 rounded-full bg-amber-100 flex items-center justify-center">
                          <AlertTriangle size={16} className="text-amber-600" />
                        </div>
                      </div>
                      <div className="ml-3">
                        <p className="text-sm font-medium text-gray-900">Temperature spike detected</p>
                        <p className="text-sm text-gray-500">2 days ago at 10:45 AM</p>
                      </div>
                    </li>
                    <li className="flex items-start">
                      <div className="flex-shrink-0">
                        <div className="h-8 w-8 rounded-full bg-green-100 flex items-center justify-center">
                          <CheckCircle size={16} className="text-green-600" />
                        </div>
                      </div>
                      <div className="ml-3">
                        <p className="text-sm font-medium text-gray-900">Maintenance check completed</p>
                        <p className="text-sm text-gray-500">1 week ago</p>
                      </div>
                    </li>
                  </ul>
                </div>
              </div>
            </div>

            {/* Right Column - Actions and Info */}
            <div className="space-y-6">
              <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
                <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                  <h3 className="text-lg font-medium leading-6 text-gray-900">Actions</h3>
                </div>
                <div className="px-4 py-5 sm:p-6 space-y-4">
                  {/* <button 
                    onClick={() => onToggleCharging(battery.esp_id)}
                    className={`w-full flex items-center justify-center px-4 py-2 border rounded-md shadow-sm text-sm font-medium focus:outline-none focus:ring-2 focus:ring-offset-2 ${
                      battery.isCharging 
                        ? 'border-red-300 text-red-700 bg-red-50 hover:bg-red-100 focus:ring-red-500' 
                        : 'border-green-300 text-green-700 bg-green-50 hover:bg-green-100 focus:ring-green-500'
                    }`}
                  >
                    {battery.isCharging ? (
                      <>
                        <Power size={16} className="mr-2" />
                        Stop Charging
                      </>
                    ) : (
                      <>
                        <Zap size={16} className="mr-2" />
                        Start Charging
                      </>
                    )}
                  </button> */}
                  <button className="w-full flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
                  onClick={() => viewDigitalTwin(battery)}>
                    <BrainCircuit size={16} className="mr-2" /> Digital Twin
                  </button>
                  <button
                    onClick={() => battery.live_websocket ? sendUnseal() : null}
                    className="w-full flex items-center justify-center px-4 py-2 border border-orange-300 shadow-sm text-sm font-medium rounded-md text-orange-700 bg-orange-50 hover:bg-orange-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-orange-500">
                    <LockKeyholeOpen size={16} className="mr-2" />
                    Unseal BMS {!battery.live_websocket ? "- OFFLINE" : ""}
                  </button>
                  <button
                    onClick={() => battery.live_websocket ? sendReset() : null}
                    className="w-full flex items-center justify-center px-4 py-2 border border-red-300 shadow-sm text-sm font-medium rounded-md text-red-700 bg-red-50 hover:bg-red-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500">
                    <RefreshCw size={16} className="mr-2" />
                    Reset BMS {!battery.live_websocket ? "- OFFLINE" : ""}
                  </button>
                  <button className="w-full flex items-center justify-center px-4 py-2 border border-gray-300 shadow-sm text-sm font-medium rounded-md text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
                    <FileText size={16} className="mr-2" />
                    Generate Report
                  </button>
                  <button 
                    className="w-full flex items-center justify-center px-4 py-2 border border-gray-300 shadow-sm text-sm font-medium rounded-md text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
                  >
                    <ArrowLeft size={16} className="mr-2" />
                    Back to List
                  </button>
                </div>
              </div>

              <div className="bg-white border border-gray-200 rounded-lg shadow-sm">
                <div className="px-4 py-5 sm:px-6 border-b border-gray-200">
                  <h3 className="text-lg font-medium leading-6 text-gray-900">Battery Information</h3>
                </div>
                <div className="px-4 py-5 sm:p-6">
                  <dl className="grid grid-cols-1 gap-x-4 gap-y-6 sm:grid-cols-2">
                    <div className="sm:col-span-1">
                      <dt className="text-sm font-medium text-gray-500">Battery ID</dt>
                      <dd className="mt-1 text-sm text-gray-900">{battery.esp_id}</dd>
                    </div>
                    {/* <div className="sm:col-span-1">
                      <dt className="text-sm font-medium text-gray-500">Type</dt>
                      <dd className="mt-1 text-sm text-gray-900">{battery.type}</dd>
                    </div> */}
                    {/* <div className="sm:col-span-1">
                      <dt className="text-sm font-medium text-gray-500">Location</dt>
                      <dd className="mt-1 text-sm text-gray-900">{battery.location}</dd>
                    </div> */}
                    <div className="sm:col-span-1">
                      <dt className="text-sm font-medium text-gray-500">Installation Date</dt>
                      <dd className="mt-1 text-sm text-gray-900">Jan 15, 2025</dd>
                    </div>
                    <div className="sm:col-span-2">
                      <dt className="text-sm font-medium text-gray-500">Notes</dt>
                      <dd className="mt-1 text-sm text-gray-900">
                        Regular maintenance performed as scheduled. No issues detected during last inspection.
                      </dd>
                    </div>
                  </dl>
                </div>
              </div>
            </div>
          </div>
        );
    }
  };

  const navigate = useNavigate();

  const viewDigitalTwin = (battery: BatteryDataNew) => {
    navigate(`/digital-twin?esp_id=${battery.esp_id}`);
  };

  return (
    <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
      {/* Battery Detail Header */}
      <div className="p-6 border-b border-gray-200 bg-gray-50">
        <div className="flex justify-between items-start">
          <div>
            <h2 className="text-2xl font-bold text-gray-800">{battery.esp_id}</h2>
            <div className="flex items-center mt-1 space-x-2 text-gray-600">
              <span>{battery.esp_id}</span>
              <span>•</span>
              {/* <span>{battery.type}</span> */}
              <span>•</span>
              {/* <span>{battery.location}</span> */}
            </div>
          </div>
          <div className="flex items-center space-x-3">
            <span className={`px-3 py-1 rounded-full text-sm font-medium ${getStatusColor(battery.live_websocket)}`}>
              { battery.live_websocket?"online":"offline" }
            </span>
            <div className="flex space-x-2">
              <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                <Share2 size={18} />
              </button>
              <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                <Download size={18} />
              </button>
              <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                <Printer size={18} />
              </button>
            </div>
          </div>
        </div>
      </div>

      {/* Battery Detail Content */}
      <div className="p-6">
        {/* Quick Stats */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
          <div className="bg-blue-50 p-4 rounded-lg">
            <div className="flex items-center justify-between mb-2">
              <h3 className="text-sm font-medium text-blue-700 flex items-center">
                {battery.Q > 90 ? (
                    <>
                      <BatteryFull size={16} className="mr-1" />
                    </>
                  ) : (
                    battery.Q > 35 ? (
                      <>
                        <BatteryMedium size={16} className="mr-1" />
                      </>
                    ) : (
                      <>
                        <BatteryLow size={16} className="mr-1" />
                      </>
                    )
                  )
                } Charge Level
              </h3>
              {/* {battery.isCharging && ( */}
              {battery.I > 0 && (
                <span className="bg-yellow-100 text-yellow-800 text-xs px-2 py-1 rounded-full flex items-center">
                  <Zap size={12} className="mr-1" /> Charging
                </span>
              )}
            </div>
            <div className="text-3xl font-bold text-blue-700">{battery.Q}%</div>
            <div className="mt-2 bg-blue-200 rounded-full h-2.5">
              <div 
                className="h-2.5 rounded-full bg-blue-600" 
                style={{ width: `${Math.min(battery.Q, 100)}%` }}
              ></div>
            </div>
          </div>

          <div className="bg-orange-50 p-4 rounded-lg">
            <h3 className="text-sm font-medium text-orange-700 flex items-center mb-2">
              <ThermometerSun size={16} className="mr-1" /> Temperature
            </h3>
            <div className="flex items-end space-x-2">
              <span className="text-3xl font-bold text-orange-700">Ambient: {battery.iT}°C</span>
              {battery.iT > 35 && (
                <span className="text-red-500 text-sm">Above normal</span>
              )}
            </div>
            <div className="flex items-end space-x-2">
              <span className="text-3xl font-bold text-orange-700">Cell: {battery.iT}°C</span>
              {battery.iT > 35 && (
                <span className="text-red-500 text-sm">Above normal</span>
              )}
            </div>
          </div>

          <div className="bg-green-50 p-4 rounded-lg">
            <h3 className="text-sm font-medium text-green-700 flex items-center mb-2">
              <Info size={16} className="mr-1" /> Battery Health
            </h3>
            <div className="flex items-end space-x-2">
              <span className="text-3xl font-bold text-green-700">{battery.H}%</span>
              {battery.H < 70 && (
                <span className="text-amber-500 text-sm">Degraded</span>
              )}
            </div>
          </div>

          <div className="bg-purple-50 p-4 rounded-lg">
            <h3 className="text-sm font-medium text-purple-700 flex items-center mb-2">
              <Wifi size={16} className="mr-1" /> Wi-Fi
            </h3>
            <div className="flex items-end space-x-2">
              <span className="text-3xl font-bold text-purple-700">{battery.isConnected? "Connected":"!! no connection"}</span>
            </div>
          </div>
        </div>

        {/* Tabs */}
        {/* Tabs */}
        <div className="border-b border-gray-200 mb-6 overflow-x-auto">
          <nav className="-mb-px flex space-x-8">
            <button
              onClick={() => setActiveTab('overview')}
              className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
                activeTab === 'overview'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              Overview
            </button>
            <button
              onClick={() => setActiveTab('performance')}
              className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
                activeTab === 'performance'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              Performance
            </button>
            <button
              onClick={() => setActiveTab('history')}
              className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
                activeTab === 'history'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              History
            </button>
            <button
              onClick={() => setActiveTab('maintenance')}
              className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
                activeTab === 'maintenance'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              Maintenance
            </button>
            <button
              onClick={() => setActiveTab('settings')}
              className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
                activeTab === 'settings'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              Settings
            </button>
            <button
              onClick={() => setActiveTab('wifi')}
              className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
                activeTab === 'wifi'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              Wi-Fi
            </button>
          </nav>
        </div>

        {/* Tab Content */}
        {renderTabContent()}
      </div>
    </div>
  );
};

export default BatteryDetail;