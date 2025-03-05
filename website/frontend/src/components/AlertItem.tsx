import React from 'react';
import { AlertData } from '../types';
import { AlertTriangle, Info } from 'lucide-react';
import { formatDateTime, getAlertTypeColor } from '../utils/helpers';

interface AlertItemProps {
  alert: AlertData;
  onMarkAsRead: (alertId: string) => void;
  onViewBattery: (batteryId: string) => void;
}

const AlertItem: React.FC<AlertItemProps> = ({ alert, onMarkAsRead, onViewBattery }) => {
  // Get alert icon based on type
  const getAlertIcon = (type: string) => {
    switch (type) {
      case 'critical': return <AlertTriangle size={20} className="text-red-600" />;
      case 'warning': return <AlertTriangle size={20} className="text-amber-600" />;
      case 'info': return <Info size={20} className="text-blue-600" />;
      default: return <Info size={20} className="text-blue-600" />;
    }
  };

  return (
    <li className={`p-4 hover:bg-gray-50 ${!alert.isRead ? 'bg-blue-50' : ''}`}>
      <div className="flex items-start">
        <div className="flex-shrink-0 mt-1">
          <div className={`h-10 w-10 rounded-full flex items-center justify-center ${
            alert.type === 'critical' ? 'bg-red-100' : 
            alert.type === 'warning' ? 'bg-amber-100' : 'bg-blue-100'
          }`}>
            {getAlertIcon(alert.type)}
          </div>
        </div>
        <div className="ml-4 flex-1">
          <div className="flex items-center justify-between">
            <h4 className="text-base font-medium text-gray-900">{alert.message}</h4>
            <span className={`px-2 py-1 text-xs rounded-full ${getAlertTypeColor(alert.type)}`}>
              {alert.type.charAt(0).toUpperCase() + alert.type.slice(1)}
            </span>
          </div>
          <div className="mt-1 text-sm text-gray-600">
            Battery: <span className="font-medium">{alert.batteryName}</span> ({alert.batteryId})
          </div>
          <div className="mt-2 flex items-center justify-between">
            <span className="text-sm text-gray-500">{formatDateTime(alert.timestamp)}</span>
            <div className="flex space-x-2">
              {!alert.isRead && (
                <button 
                  onClick={() => onMarkAsRead(alert.id)}
                  className="text-xs text-blue-600 hover:text-blue-800"
                >
                  Mark as read
                </button>
              )}
              <button 
                onClick={() => onViewBattery(alert.batteryId)}
                className="text-xs text-blue-600 hover:text-blue-800"
              >
                View battery
              </button>
            </div>
          </div>
        </div>
      </div>
    </li>
  );
};

export default AlertItem;