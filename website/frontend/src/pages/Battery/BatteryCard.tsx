import React from 'react';
import { BatteryInfoData, BatteryDataNew } from '../..//types';
import { getStatusColor} from '../../utils/helpers';
import { format } from "date-fns";

interface BatteryCardProps {
  battery: BatteryDataNew;
  viewBatteryDetails: (battery: BatteryInfoData) => void;
}


const BatteryCard: React.FC<BatteryCardProps> = ({ battery, viewBatteryDetails }) => {

  return (
    <div key={battery.esp_id} className="bg-white rounded-lg shadow p-4">
      <div className="flex justify-between items-start mb-3">
        <div>
          <h2 className="text-lg font-semibold">{battery.esp_id}</h2>
          <p className="text-sm text-gray-600">Last Updated: {format(new Date(battery.last_updated_time), "yyyy-MM-dd HH:mm:ss")}</p>
        </div>
        <div className="flex items-center gap-2">
            <a
              href="#"
              onClick={(e) => {
                e.preventDefault(); 
                viewBatteryDetails(battery);
              }}
              className="cursor-pointer px-2 py-0.5 bg-blue-600 text-white rounded hover:bg-blue-700 transition-colors text-xs"
            >
              Details
            </a>
          <span className={`px-2 py-0.5 rounded-full text-xs font-medium ${getStatusColor(battery.live_websocket)}`}>
            {battery.live_websocket?"online":"offline"}
          </span>
        </div>
      </div>

      <div className="grid grid-cols-2 gap-2 mb-3">
        <div className="bg-gray-50 p-2 rounded">
          <p className="text-xs text-gray-600">temperature</p>
          <p className="text-sm font-semibold">{battery.cT}°C</p>
        </div>
        <div className="bg-gray-50 p-2 rounded">
          <p className="text-xs text-gray-600">Voltage</p>
          <p className="text-sm font-semibold">{battery.V}V</p>
        </div>
      </div>

      <div className="space-y-2">
        <div className="flex justify-between items-center">
          <h3 className="text-sm font-medium text-gray-700">Connected Batteries</h3>
          <span className="text-xs text-gray-500">
            {battery.nodes?.length || 0} units
          </span>
        </div>
        <div className={`space-y-2 ${battery.nodes && battery.nodes.length > 3 ? 'max-h-[200px] overflow-y-auto pr-1 custom-scrollbar' : ''}`}>
          {battery.nodes?.map((childBattery) => (
            <div 
              onClick={() => viewBatteryDetails(childBattery)}
              key={childBattery.esp_id}
              className="block p-2 bg-gray-50 rounded hover:bg-gray-100 transition-colors"
            >
              <div className="flex justify-between items-center">
                <div>
                  <h4 className="text-sm font-medium">{childBattery.esp_id}</h4>
                  {/* <div className="flex items-center gap-2 mt-0.5">
                    <span className="text-xs text-gray-600">
                      {childBattery.cT}°C
                    </span>
                    <span className="text-xs text-gray-600">
                      {childBattery.V}V
                    </span>
                  </div> */}
                </div>
                <span className={`px-1.5 py-0.5 rounded-full text-xs font-medium ${getStatusColor(childBattery.live_websocket)}`}>
                  {childBattery.live_websocket?"online":"offline"}
                </span>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default BatteryCard;