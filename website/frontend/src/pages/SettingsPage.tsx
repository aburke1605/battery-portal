import React from 'react';
import Breadcrumb from '../components/Breadcrumb';
import { CheckCircle } from 'lucide-react';

interface SettingsPageProps {
  temperatureThreshold: number;
  setTemperatureThreshold: (value: number) => void;
  voltageThreshold: number;
  setVoltageThreshold: (value: number) => void;
  healthThreshold: number;
  setHealthThreshold: (value: number) => void;
}

const SettingsPage: React.FC<SettingsPageProps> = ({
  temperatureThreshold,
  setTemperatureThreshold,
  voltageThreshold,
  setVoltageThreshold,
  healthThreshold,
  setHealthThreshold
}) => {
  return (
    <>
      {/* Breadcrumbs */}
      <Breadcrumb pageName="Settings" />

      {/* Settings Header */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
        <h2 className="text-xl font-semibold text-gray-800 mb-1">System Settings</h2>
        <p className="text-sm text-gray-600">
          Configure system parameters and thresholds
        </p>
      </div>

      {/* Settings Tabs */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 overflow-hidden mb-6">
        <div className="border-b border-gray-200">
          <nav className="-mb-px flex space-x-8 px-6">
            <a href="#" className="border-blue-500 text-blue-600 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              System Thresholds
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              Notifications
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              User Permissions
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              Maintenance Schedule
            </a>
          </nav>
        </div>

        <div className="p-6">
          <div className="max-w-3xl">
            <h3 className="text-lg font-medium text-gray-900 mb-4">Alert Thresholds</h3>
            <p className="text-sm text-gray-600 mb-6">
              Configure when the system should generate alerts based on battery parameters.
            </p>

            <div className="space-y-6">
              <div>
                <label htmlFor="temperature-threshold" className="block text-sm font-medium text-gray-700 mb-1">
                  Temperature Threshold (°C)
                </label>
                <div className="flex items-center">
                  <input
                    type="range"
                    id="temperature-threshold"
                    min="30"
                    max="60"
                    step="1"
                    value={temperatureThreshold}
                    onChange={(e) => setTemperatureThreshold(parseInt(e.target.value))}
                    className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <span className="ml-4 text-sm font-medium text-gray-900 min-w-[40px]">
                    {temperatureThreshold}°C
                  </span>
                </div>
                <p className="mt-1 text-xs text-gray-500">
                  Alert will be triggered when battery temperature exceeds this value.
                </p>
              </div>

              <div>
                <label htmlFor="voltage-threshold" className="block text-sm font-medium text-gray-700 mb-1">
                  Minimum Voltage Threshold (V)
                </label>
                <div className="flex items-center">
                  <input
                    type="range"
                    id="voltage-threshold"
                    min="44"
                    max="48"
                    step="0.1"
                    value={voltageThreshold}
                    onChange={(e) => setVoltageThreshold(parseFloat(e.target.value))}
                    className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <span className="ml-4 text-sm font-medium text-gray-900 min-w-[40px]">
                    {voltageThreshold}V
                  </span>
                </div>
                <p className="mt-1 text-xs text-gray-500">
                  Alert will be triggered when battery voltage falls below this value.
                </p>
              </div>

              <div>
                <label htmlFor="health-threshold" className="block text-sm font-medium text-gray-700 mb-1">
                  Health Threshold (%)
                </label>
                <div className="flex items-center">
                  <input
                    type="range"
                    id="health-threshold"
                    min="60"
                    max="90"
                    step="1"
                    value={healthThreshold}
                    onChange={(e) => setHealthThreshold(parseInt(e.target.value))}
                    className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <span className="ml-4 text-sm font-medium text-gray-900 min-w-[40px]">
                    {healthThreshold}%
                  </span>
                </div>
                <p className="mt-1 text-xs text-gray-500">
                  Alert will be triggered when battery health falls below this value.
                </p>
              </div>

              <div className="pt-4">
                <button className="px-4 py-2 bg-blue-600 text-white rounded-md text-sm font-medium hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
                  Save Settings
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* System Information */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6">
        <h3 className="text-lg font-medium text-gray-900 mb-4">System Information</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          <div>
            <h4 className="text-sm font-medium text-gray-500 mb-2">Software Version</h4>
            <p className="text-base text-gray-900">Battery Management System v2.5.1</p>
          </div>
          <div>
            <h4 className="text-sm font-medium text-gray-500 mb-2">Last Updated</h4>
            <p className="text-base text-gray-900">March 25, 2025</p>
          </div>
          <div>
            <h4 className="text-sm font-medium text-gray-500 mb-2">Database Status</h4>
            <p className="text-base text-gray-900 flex items-center">
              <CheckCircle size={16} className="text-green-500 mr-2" />
              Connected
            </p>
          </div>
          <div>
            <h4 className="text-sm font-medium text-gray-500 mb-2">API Status</h4>
            <p className="text-base text-gray-900 flex items-center">
              <CheckCircle size={16} className="text-green-500 mr-2" />
              Operational
            </p>
          </div>
        </div>
      </div>
    </>
  );
};

export default SettingsPage;