import React from 'react';
import { BatteryData } from '../..//types';
import { 
  ToggleRight,
  ToggleLeft,
  ExternalLink
} from 'lucide-react';
import { getStatusColor, getChargeLevelColor, getHealthColor, getTemperatureColor } from '../../utils/helpers';

interface BatteryCardProps {
  battery: BatteryData;
  onToggleCharging: (batteryId: string, e?: React.MouseEvent) => void;
  onViewDetails: (battery: BatteryData) => void;
}

const BatteryCard: React.FC<BatteryCardProps> = ({ battery, onToggleCharging, onViewDetails }) => {
  // Get battery icon based on charge level and status
  // const getBatteryIcon = (battery: BatteryData) => {
  //   if (battery.status === 'offline') return <Battery className="text-gray-400" />;
  //   if (battery.status === 'critical') return <BatteryWarning className="text-red-500" />;
  //   if (battery.status === 'warning') return <BatteryLow className="text-amber-500" />;
    
  //   if (battery.chargeLevel > 80) return <BatteryFull className="text-green-500" />;
  //   if (battery.chargeLevel > 40) return <BatteryMedium className="text-blue-500" />;
  //   return <BatteryLow className="text-amber-500" />;
  // };

  return (
    <div 
      className="bg-white rounded-lg shadow-sm border border-gray-200 overflow-hidden hover:shadow-md transition-shadow"
    >
      <div className="p-5">
        <div className="flex justify-between items-start mb-3">
          <h3 className="text-lg font-semibold text-gray-900">{battery.esp_id}</h3>
          <span className={`text-xs px-2 py-1 rounded-full ${getStatusColor(battery.status)}`}>
            {battery.status.charAt(0).toUpperCase() + battery.status.slice(1)}
          </span>
        </div>
        
        <div className="text-sm text-gray-600 mb-1">ID: {battery.esp_id}</div>
        <div className="text-sm text-gray-600 mb-4">Location: {battery.location}</div>
        
        <div className="mb-4">
          <div className="flex justify-between items-center mb-1">
            <span className="text-sm font-medium text-gray-700">Charge Level</span>
            <span className="text-sm font-medium">{battery.charge}%</span>
          </div>
          <div className="w-full bg-gray-200 rounded-full h-2.5">
            <div 
              className={`h-2.5 rounded-full ${getChargeLevelColor(battery.charge, battery.status)}`}
              style={{ width: `${battery.charge}%` }}
            ></div>
          </div>
        </div>
        
        <div className="grid grid-cols-2 gap-4">
          <div>
            <div className="text-sm text-gray-500">Temperature:</div>
            <div className={`font-medium ${getTemperatureColor(battery.temperature)}`}>{battery.temperature}°C</div>
          </div>
          <div>
            <div className="text-sm text-gray-500">Voltage:</div>
            <div className="font-medium text-gray-700">{battery.voltage}V</div>
          </div>
          <div>
            <div className="text-sm text-gray-500">Health:</div>
            <div className={`font-medium ${getHealthColor(battery.health)}`}>{battery.health}%</div>
          </div>
          <div>
            <div className="text-sm text-gray-500">Cycles:</div>
            <div className="font-medium text-gray-700">{battery.cycleCount}</div>
          </div>
        </div>

        <div className="mt-4 flex justify-between items-center">
          <button 
            onClick={(e) => onToggleCharging(battery.esp_id, e)}
            className="flex items-center text-gray-600 hover:text-gray-900"
          >
            {battery.isCharging ? (
              <>
                <ToggleRight size={18} className="text-green-500 mr-1" />
                <span className="text-sm">Charging</span>
              </>
            ) : (
              <>
                <ToggleLeft size={18} className="text-gray-400 mr-1" />
                <span className="text-sm">Not Charging</span>
              </>
            )}
          </button>
          <button 
            onClick={() => onViewDetails(battery)}
            className="flex items-center text-blue-600 hover:text-blue-800"
          >
            <span className="text-sm mr-1">Details</span>
            <ExternalLink size={16} />
          </button>
        </div>
      </div>
    </div>
  );
};

export default BatteryCard;