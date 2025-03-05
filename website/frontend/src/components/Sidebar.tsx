import React from 'react';
import { 
  BatteryCharging, 
  LayoutDashboard, 
  Gauge, 
  Bell, 
  FileText, 
  Users, 
  Settings, 
  LogOut 
} from 'lucide-react';

interface SidebarProps {
  activePage: string;
  setActivePage: (page: string) => void;
  sidebarOpen: boolean;
  setSidebarOpen: (open: boolean) => void;
  unreadAlertsCount: number;
}

const Sidebar: React.FC<SidebarProps> = ({ 
  activePage, 
  setActivePage, 
  sidebarOpen, 
  setSidebarOpen,
  unreadAlertsCount
}) => {
  // Navigation items
  const navItems = [
    { name: 'Dashboard', icon: <LayoutDashboard size={20} />, id: 'dashboard' },
    { name: 'Batteries', icon: <BatteryCharging size={20} />, id: 'batteries' },
    { name: 'Monitoring', icon: <Gauge size={20} />, id: 'monitoring' },
    { name: 'Alerts', icon: <Bell size={20} />, id: 'alerts', badge: unreadAlertsCount },
    { name: 'Reports', icon: <FileText size={20} />, id: 'reports' },
    { name: 'Users', icon: <Users size={20} />, id: 'users' },
    { name: 'Settings', icon: <Settings size={20} />, id: 'settings' },
  ];

  return (
    <div 
      className={`fixed inset-y-0 left-0 z-20 w-64 bg-[#1C2434] transform transition-transform duration-300 ease-in-out lg:translate-x-0 ${
        sidebarOpen ? 'translate-x-0' : '-translate-x-full'
      }`}
    >
      {/* Sidebar header */}
      <div className="h-16 flex items-center justify-center border-b border-gray-700">
        <h1 className="text-xl font-bold text-white flex items-center">
          <BatteryCharging className="mr-2" />
          BatteryMonitor
        </h1>
      </div>

      {/* Sidebar content */}
      <div className="py-4 h-[calc(100vh-4rem-4rem)] overflow-y-auto">
        <nav className="mt-5 px-2 space-y-1">
          {navItems.map((item) => (
            <a
              key={item.id}
              href="#"
              onClick={() => {
                setActivePage(item.id);
                setSidebarOpen(false);
              }}
              className={`group flex items-center px-4 py-3 text-sm font-medium rounded-md transition-all duration-200 ${
                activePage === item.id
                  ? 'bg-blue-600 text-white shadow-md relative pl-6 before:absolute before:left-0 before:top-0 before:bottom-0 before:w-1 before:bg-blue-300'
                  : 'text-[rgb(222,228,238)] hover:bg-gray-700 hover:text-white hover:translate-x-1'
              }`}
            >
              <span className={`mr-3 transition-transform duration-200 ${activePage !== item.id ? 'group-hover:scale-110' : ''}`}>{item.icon}</span>
              {item.name}
              {item.badge && item.badge > 0 && (
                <span className="ml-auto bg-red-500 text-white text-xs px-2 py-0.5 rounded-full">
                  {item.badge}
                </span>
              )}
            </a>
          ))}
        </nav>
      </div>

      {/* Sidebar footer */}
      <div className="absolute bottom-0 w-full border-t border-gray-700 p-4">
        <div className="flex items-center">
          <div className="h-8 w-8 rounded-full bg-blue-500 flex items-center justify-center text-white font-medium">
            JD
          </div>
          <div className="ml-3">
            <p className="text-sm font-medium text-white">John Doe</p>
            <p className="text-xs text-gray-400">Administrator</p>
          </div>
          <button className="ml-auto p-1 rounded-full text-gray-400 hover:text-white hover:bg-gray-700 transition-colors duration-200">
            <LogOut size={18} />
          </button>
        </div>
      </div>
    </div>
  );
};

export default Sidebar;