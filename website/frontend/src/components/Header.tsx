import React from 'react';
import { Bell, HelpCircle, Search, BatteryCharging } from 'lucide-react';

interface HeaderProps {
  activePage: string;
  searchTerm: string;
  setSearchTerm: (term: string) => void;
  unreadAlertsCount: number;
  navItems: Array<{id: string, name: string}>;
}

const Header: React.FC<HeaderProps> = ({ 
  activePage, 
  searchTerm, 
  setSearchTerm, 
  unreadAlertsCount,
  navItems
}) => {
  return (
    <header className="bg-white shadow-sm z-10">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex justify-between h-16 items-center">
          <div className="flex items-center">
            <div className="flex-shrink-0 mr-4">
              <BatteryCharging size={28} className="text-blue-600" />
            </div>
            <h1 className="text-xl font-semibold text-gray-900">
              {navItems.find(item => item.id === activePage)?.name || 'Dashboard'}
            </h1>
          </div>
          <div className="flex items-center">
            <div className="relative hidden sm:block">
              <input
                type="text"
                placeholder="Search..."
                className="w-64 px-4 py-2 pr-8 rounded-md border border-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500"
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
              />
              <Search className="absolute right-3 top-2.5 text-gray-400" size={18} />
            </div>
            <button className="ml-4 p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full relative">
              <Bell size={20} />
              {unreadAlertsCount > 0 && (
                <span className="absolute top-1 right-1 h-4 w-4 bg-red-500 rounded-full text-xs text-white flex items-center justify-center">
                  {unreadAlertsCount}
                </span>
              )}
            </button>
            <button className="ml-2 p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
              <HelpCircle size={20} />
            </button>
          </div>
        </div>
      </div>
    </header>
  );
};

export default Header;