import React from 'react';
import { BatteryData } from '../types';
import { 
  Share2, 
  Download, 
  Printer, 
  Percent, 
  Zap, 
  ThermometerSun, 
  Info, 
  RefreshCw, 
  Power, 
  Calendar, 
  FileText, 
  ArrowLeft, 
  History, 
  BarChart3, 
  AlertTriangle, 
  CheckCircle 
} from 'lucide-react';
import { getStatusColor, getHealthColor } from '../utils/helpers';

interface BatteryDetailProps {
  battery: BatteryData;
  onToggleCharging: (batteryId: string) => void;
  onBackToList: () => void;
  voltageThreshold: number;
}

const BatteryDetail: React.FC<BatteryDetailProps> = ({ 
  battery, 
  onToggleCharging, 
  onBackToList,
  voltageThreshold
}) => {
  return (
    <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
      {/* Battery Detail Header */}
      <div className="p-6 border-b border-gray-200 bg-gray-50">
        <div className="flex justify-between items-start">
          <div>
            <h2 className="text-2xl font-bold text-gray-800">{battery.name}</h2>
            <div className="flex items-center mt-1 space-x-2 text-gray-600">
              <span>{battery.id}</span>
              <span>•</span>
              <span>{battery.type}</span>
              <span>•</span>
              <span>{battery.location}</span>
            </div>
          </div>
          <div className="flex items-center space-x-3">
            <span className={`px-3 py-1 rounded-full text-sm font-medium ${getStatusColor(battery.status)}`}>
              {battery.status.charAt(0).toUpperCase() + battery.status.slice(1)}
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
                <Percent size={16} className="mr-1" /> Charge Level
              </h3>
              {battery.isCharging && (
                <span className="bg-yellow-100 text-yellow-800 text-xs px-2 py-1 rounded-full flex items-center">
                  <Zap size={12} className="mr-1" /> Charging
                </span>
              )}
            </div>
            <div className="text-3xl font-bold text-blue-700">{battery.chargeLevel}%</div>
            <div className="mt-2 bg-blue-200 rounded-full h-2.5">
              <div 
                className="h-2.5 rounded-full bg-blue-600" 
                style={{ width: `${battery.chargeLevel}%` }}
              ></div>
            </div>
          </div>

          <div className="bg-orange-50 p-4 rounded-lg">
            <h3 className="text-sm font-medium text-orange-700 flex items-center mb-2">
              <ThermometerSun size={16} className="mr-1" /> Temperature
            </h3>
            <div className="flex items-end space-x-2">
              <span className="text-3xl font-bold text-orange-700">{battery.temperature}°C</span>
              {battery.temperature > 35 && (
                <span className="text-red-500 text-sm">Above normal</span>
              )}
            </div>
          </div>

          <div className="bg-green-50 p-4 rounded-lg">
            <h3 className="text-sm font-medium text-green-700 flex items-center mb-2">
              <Info size={16} className="mr-1" /> Battery Health
            </h3>
            <div className="flex items-end space-x-2">
              <span className="text-3xl font-bold text-green-700">{battery.health}%</span>
              {battery.health < 70 && (
                <span className="text-amber-500 text-sm">Degraded</span>
              )}
            </div>
          </div>

          <div className="bg-purple-50 p-4 rounded-lg">
            <h3 className="text-sm font-medium text-purple-700 flex items-center mb-2">
              <RefreshCw size={16} className="mr-1" /> Cycle Count
            </h3>
            <span className="text-3xl font-bold text-purple-700">{battery.cycleCount}</span>
          </div>
        </div>

        {/* Tabs */}
        <div className="border-b border-gray-200 mb-6 overflow-x-auto">
          <nav className="-mb-px flex space-x-8">
            <a href="#" className="border-blue-500 text-blue-600 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              Overview
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              Performance
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              History
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              Maintenance
            </a>
            <a href="#" className="border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300 whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm">
              Settings
            </a>
          </nav>
        </div>

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
                    <p className="text-2xl font-semibold text-gray-900">{battery.voltage} V</p>
                    <div className="mt-1 text-sm text-gray-500">
                      {battery.voltage < voltageThreshold ? (
                        <span className="text-amber-600">Below threshold ({voltageThreshold}V)</span>
                      ) : (
                        <span className="text-green-600">Normal operating range</span>
                      )}
                    </div>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Current</h4>
                    <p className="text-2xl font-semibold text-gray-900">{battery.current} A</p>
                    <div className="mt-1 text-sm text-gray-500">
                      {battery.isCharging ? (
                        <span className="text-blue-600">Charging current</span>
                      ) : (
                        <span>Discharge current</span>
                      )}
                    </div>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Capacity</h4>
                    <p className="text-2xl font-semibold text-gray-900">{battery.capacity} kWh</p>
                    <div className="mt-1 text-sm text-gray-500">
                      Nominal capacity
                    </div>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Last Maintenance</h4>
                    <p className="text-2xl font-semibold text-gray-900">{battery.lastMaintenance}</p>
                    <div className="mt-1 text-sm text-gray-500">
                      {new Date(battery.lastMaintenance) < new Date(Date.now() - 90 * 24 * 60 * 60 * 1000) ? (
                        <span className="text-amber-600">Maintenance due</span>
                      ) : (
                        <span className="text-green-600">Up to date</span>
                      )}
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
                <button 
                  onClick={() => onToggleCharging(battery.id)}
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
                </button>
                <button className="w-full flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
                  <Calendar size={16} className="mr-2" />
                  Schedule Maintenance
                </button>
                <button className="w-full flex items-center justify-center px-4 py-2 border border-gray-300 shadow-sm text-sm font-medium rounded-md text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
                  <FileText size={16} className="mr-2" />
                  Generate Report
                </button>
                <button 
                  onClick={onBackToList}
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
                    <dd className="mt-1 text-sm text-gray-900">{battery.id}</dd>
                  </div>
                  <div className="sm:col-span-1">
                    <dt className="text-sm font-medium text-gray-500">Type</dt>
                    <dd className="mt-1 text-sm text-gray-900">{battery.type}</dd>
                  </div>
                  <div className="sm:col-span-1">
                    <dt className="text-sm font-medium text-gray-500">Location</dt>
                    <dd className="mt-1 text-sm text-gray-900">{battery.location}</dd>
                  </div>
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
      </div>
    </div>
  );
};

export default BatteryDetail;