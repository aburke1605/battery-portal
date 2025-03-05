import React from 'react';
import { AlertData, BatteryData } from '../types';
import Breadcrumb from '../components/Breadcrumb';
import AlertItem from '../components/AlertItem';
import { Bell, SlidersHorizontal, ChevronDown } from 'lucide-react';

interface AlertsPageProps {
  alerts: AlertData[];
  batteries: BatteryData[];
  markAlertAsRead: (alertId: string) => void;
  viewBatteryDetails: (battery: BatteryData) => void;
  setActivePage: (page: string) => void;
}

const AlertsPage: React.FC<AlertsPageProps> = ({ 
  alerts, 
  batteries, 
  markAlertAsRead, 
  viewBatteryDetails,
  setActivePage
}) => {
  const handleViewBattery = (batteryId: string) => {
    const battery = batteries.find(b => b.id === batteryId);
    if (battery) {
      viewBatteryDetails(battery);
      setActivePage('batteries');
    }
  };

  return (
    <>
      {/* Breadcrumbs */}
      <Breadcrumb pageName="Alerts" />

      {/* Alerts Header */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between">
          <div>
            <h2 className="text-xl font-semibold text-gray-800 mb-1">System Alerts</h2>
            <p className="text-sm text-gray-600">
              {alerts.filter(a => !a.isRead).length} unread alerts
            </p>
          </div>
          <div className="mt-4 md:mt-0 flex flex-wrap gap-2">
            <button className="px-4 py-2 bg-blue-600 text-white rounded-md text-sm font-medium hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 flex items-center">
              <Bell size={16} className="mr-2" />
              Mark All as Read
            </button>
            <button className="px-4 py-2 border border-gray-300 bg-white text-gray-700 rounded-md text-sm font-medium hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 flex items-center">
              <SlidersHorizontal size={16} className="mr-2" />
              Alert Settings
            </button>
          </div>
        </div>
      </div>

      {/* Alerts List */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 overflow-hidden">
        <div className="px-4 py-5 sm:px-6 border-b border-gray-200 bg-gray-50 flex justify-between items-center">
          <h3 className="text-lg font-medium leading-6 text-gray-900">All Alerts</h3>
          <div className="flex items-center space-x-2">
            <div className="relative">
              <select 
                className="appearance-none bg-white border rounded-md px-4 py-2 pr-8 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 text-sm"
              >
                <option value="all">All Types</option>
                <option value="critical">Critical</option>
                <option value="warning">Warning</option>
                <option value="info">Info</option>
              </select>
              <ChevronDown size={16} className="absolute right-2 top-2.5 text-gray-500 pointer-events-none" />
            </div>
            <div className="relative">
              <select 
                className="appearance-none bg-white border rounded-md px-4 py-2 pr-8 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 text-sm"
              >
                <option value="newest">Newest First</option>
                <option value="oldest">Oldest First</option>
              </select>
              <ChevronDown size={16} className="absolute right-2 top-2.5 text-gray-500 pointer-events-none" />
            </div>
          </div>
        </div>
        <ul className="divide-y divide-gray-200">
          {alerts.map(alert => (
            <AlertItem 
              key={alert.id} 
              alert={alert} 
              onMarkAsRead={markAlertAsRead} 
              onViewBattery={handleViewBattery} 
            />
          ))}
        </ul>
      </div>
    </>
  );
};

export default AlertsPage;