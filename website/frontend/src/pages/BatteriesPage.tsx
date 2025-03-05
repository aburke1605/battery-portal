import React from 'react';
import { BatteryData } from '../types';
import Breadcrumb from '../components/Breadcrumb';
import BatteryCard from '../components/BatteryCard';
import BatteryDetail from '../components/BatteryDetail';
import { ChevronDown } from 'lucide-react';

interface BatteriesPageProps {
  batteries: BatteryData[];
  filteredBatteries: BatteryData[];
  selectedBattery: BatteryData | null;
  statusFilter: string;
  setStatusFilter: (status: string) => void;
  sortBy: string;
  setSortBy: (sort: string) => void;
  view: 'list' | 'detail';
  toggleCharging: (batteryId: string, e?: React.MouseEvent) => void;
  viewBatteryDetails: (battery: BatteryData) => void;
  backToList: () => void;
  getSystemHealth: () => string;
  getSystemHealthColor: () => string;
  getAverageChargeLevel: () => number;
  voltageThreshold: number;
}

const BatteriesPage: React.FC<BatteriesPageProps> = ({
  batteries,
  filteredBatteries,
  selectedBattery,
  statusFilter,
  setStatusFilter,
  sortBy,
  setSortBy,
  view,
  toggleCharging,
  viewBatteryDetails,
  backToList,
  getSystemHealth,
  getSystemHealthColor,
  getAverageChargeLevel,
  voltageThreshold
}) => {
  if (view === 'list') {
    return (
      <>
        {/* Breadcrumbs */}
        <Breadcrumb pageName="Batteries" />

        {/* System Summary */}
        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
          <h2 className="text-xl font-semibold text-gray-800 mb-4">System Summary</h2>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
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
              <div className="text-sm text-gray-500 mb-1">Alerts</div>
              <div className="text-2xl font-bold text-gray-800">
                {batteries.filter(b => b.status === 'warning' || b.status === 'critical').length}
              </div>
            </div>
          </div>
        </div>

        {/* Filter Tabs and Sort */}
        <div className="flex flex-col md:flex-row md:justify-between md:items-center mb-6 space-y-4 md:space-y-0">
          <div className="flex flex-wrap gap-2">
            <button 
              className={`px-4 py-2 rounded-full text-sm font-medium ${statusFilter === 'all' ? 'bg-blue-100 text-blue-800' : 'bg-gray-100 text-gray-800 hover:bg-gray-200'}`}
              onClick={() => setStatusFilter('all')}
            >
              All Batteries
            </button>
            <button 
              className={`px-4 py-2 rounded-full text-sm font-medium ${statusFilter === 'good' ? 'bg-green-100 text-green-800' : 'bg-gray-100 text-gray-800 hover:bg-gray-200'}`}
              onClick={() => setStatusFilter('good')}
            >
              Good
            </button>
            <button 
              className={`px-4 py-2 rounded-full text-sm font-medium ${statusFilter === 'warning' ? 'bg-amber-100 text-amber-800' : 'bg-gray-100 text-gray-800 hover:bg-gray-200'}`}
              onClick={() => setStatusFilter('warning')}
            >
              Warning
            </button>
            <button 
              className={`px-4 py-2 rounded-full text-sm font-medium ${statusFilter === 'critical' ? 'bg-red-100 text-red-800' : 'bg-gray-100 text-gray-800 hover:bg-gray-200'}`}
              onClick={() => setStatusFilter('critical')}
            >
              Critical
            </button>
            <button 
              className={`px-4 py-2 rounded-full text-sm font-medium ${statusFilter === 'offline' ? 'bg-gray-200 text-gray-800' : 'bg-gray-100 text-gray-800 hover:bg-gray-200'}`}
              onClick={() => setStatusFilter('offline')}
            >
              Offline
            </button>
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-sm text-gray-600">Sort by:</span>
            <div className="relative">
              <select 
                className="appearance-none bg-white border rounded-md px-4 py-2 pr-8 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 text-sm"
                value={sortBy}
                onChange={(e) => setSortBy(e.target.value)}
              >
                <option value="status">Status</option>
                <option value="name">Name</option>
                <option value="chargeLevel">Charge Level</option>
                <option value="health">Health</option>
              </select>
              <ChevronDown size={16} className="absolute right-2 top-2.5 text-gray-500 pointer-events-none" />
            </div>
          </div>
        </div>

        {/* Battery Grid */}
        <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-6">
          {filteredBatteries.map(battery => (
            <BatteryCard 
              key={battery.id} 
              battery={battery} 
              onToggleCharging={toggleCharging} 
              onViewDetails={viewBatteryDetails} 
            />
          ))}
        </div>
      </>
    );
  } else if (selectedBattery) {
    return (
      <BatteryDetail 
        battery={selectedBattery} 
        onToggleCharging={toggleCharging} 
        onBackToList={backToList}
        voltageThreshold={voltageThreshold}
      />
    );
  }
  return null;
};

export default BatteriesPage;