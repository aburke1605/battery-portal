import React from 'react';
import { BatteryData, AlertData } from '../types';
import Breadcrumb from '../components/Breadcrumb';
import { 
  BatteryCharging, 
  AlertTriangle, 
  FileText, 
  BarChart3, 
  ChevronRight, 
  CheckCircle 
} from 'lucide-react';
import { formatDateTime, getAlertTypeColor } from '../utils/helpers';

interface DashboardPageProps {
  batteries: BatteryData[];
  alerts: AlertData[];
  setActivePage: (page: string) => void;
  setStatusFilter: (status: string) => void;
  getSystemHealth: () => string;
  getSystemHealthColor: () => string;
  getAverageChargeLevel: () => number;
  getAlertIcon: (type: string) => JSX.Element;
}

const DashboardPage: React.FC<DashboardPageProps> = ({ 
  batteries, 
  alerts, 
  setActivePage, 
  setStatusFilter,
  getSystemHealth,
  getSystemHealthColor,
  getAverageChargeLevel,
  getAlertIcon
}) => {
  return (
    <>
      {/* Breadcrumbs */}
      <Breadcrumb pageName="Dashboard" />

      {/* Dashboard Content */}
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

      {/* Recent Alerts */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
        <div className="flex justify-between items-center mb-4">
          <h2 className="text-xl font-semibold text-gray-800">Recent Alerts</h2>
          <button 
            className="text-blue-600 hover:text-blue-800 text-sm font-medium"
            onClick={() => setActivePage('alerts')}
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

      {/* Battery Status Summary */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mb-6">
        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6">
          <h2 className="text-lg font-semibold text-gray-800 mb-4">Battery Status</h2>
          <div className="space-y-4">
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Good</span>
              <span className="text-sm font-medium text-gray-800">
                {batteries.filter(b => b.status === 'good').length}
              </span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Warning</span>
              <span className="text-sm font-medium text-amber-600">
                {batteries.filter(b => b.status === 'warning').length}
              </span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Critical</span>
              <span className="text-sm font-medium text-red-600">
                {batteries.filter(b => b.status === 'critical').length}
              </span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Offline</span>
              <span className="text-sm font-medium text-gray-500">
                {batteries.filter(b => b.status === 'offline').length}
              </span>
            </div>
          </div>
        </div>

        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6">
          <h2 className="text-lg font-semibold text-gray-800 mb-4">Charging Status</h2>
          <div className="space-y-4">
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Currently Charging</span>
              <span className="text-sm font-medium text-green-600">
                {batteries.filter(b => b.isCharging).length}
              </span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Not Charging</span>
              <span className="text-sm font-medium text-gray-800">
                {batteries.filter(b => !b.isCharging).length}
              </span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Low Battery (&lt;20%)</span>
              <span className="text-sm font-medium text-red-600">
                {batteries.filter(b => b.chargeLevel < 20).length}
              </span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-sm text-gray-600">Fully Charged (&gt;90%)</span>
              <span className="text-sm font-medium text-green-600">
                {batteries.filter(b => b.chargeLevel > 90).length}
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* Quick Access */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6">
        <h2 className="text-lg font-semibold text-gray-800 mb-4">Quick Access</h2>
        <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-4">
          <button 
            className="p-4 bg-blue-50 rounded-lg hover:bg-blue-100 transition-colors flex flex-col items-center justify-center"
            onClick={() => setActivePage('batteries')}
          >
            <BatteryCharging size={24} className="text-blue-600 mb-2" />
            <span className="text-sm font-medium text-gray-800">View All Batteries</span>
          </button>
          <button 
            className="p-4 bg-amber-50 rounded-lg hover:bg-amber-100 transition-colors flex flex-col items-center justify-center"
            onClick={() => {
              setActivePage('batteries');
              setStatusFilter('warning');
            }}
          >
            <AlertTriangle size={24} className="text-amber-600 mb-2" />
            <span className="text-sm font-medium text-gray-800">Warning Batteries</span>
          </button>
          <button 
            className="p-4 bg-red-50 rounded-lg hover:bg-red-100 transition-colors flex flex-col items-center justify-center"
            onClick={() => {
              setActivePage('batteries');
              setStatusFilter('critical');
            }}
          >
            <AlertTriangle size={24} className="text-red-600 mb-2" />
            <span className="text-sm font-medium text-gray-800">Critical Batteries</span>
          </button>
          <button 
            className="p-4 bg-green-50 rounded-lg hover:bg-green-100 transition-colors flex flex-col items-center justify-center"
            onClick={() => setActivePage('reports')}
          >
            <FileText size={24} className="text-green-600 mb-2" />
            <span className="text-sm font-medium text-gray-800">Generate Report</span>
          </button>
        </div>
      </div>
    </>
  );
};

export default DashboardPage;