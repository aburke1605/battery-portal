import React, { useState, useEffect } from 'react';
import { AlertTriangle, Info, Menu, X } from 'lucide-react';
import { initialBatteries, initialAlerts, initialUsers } from './data/mockData';
import { BatteryData, AlertData } from './types';
import Sidebar from './components/Sidebar';
import Header from './components/Header';
import Footer from './components/Footer';
import DashboardPage from './pages/DashboardPage';
import BatteriesPage from './pages/BatteriesPage';
import AlertsPage from './pages/AlertsPage';
import ReportsPage from './pages/ReportsPage';
import UsersPage from './pages/UsersPage';
import SettingsPage from './pages/SettingsPage';

function App() {
  // State for batteries and alerts
  const [batteries, setBatteries] = useState<BatteryData[]>(initialBatteries);
  const [alerts, setAlerts] = useState<AlertData[]>(initialAlerts);
  const [users] = useState(initialUsers);
  
  // UI state
  const [activePage, setActivePage] = useState('dashboard');
  const [sidebarOpen, setSidebarOpen] = useState(false);
  const [searchTerm, setSearchTerm] = useState('');
  const [statusFilter, setStatusFilter] = useState('all');
  const [sortBy, setSortBy] = useState('status');
  const [selectedBattery, setSelectedBattery] = useState<BatteryData | null>(null);
  const [view, setView] = useState<'list' | 'detail'>('list');
  
  // Settings
  const [temperatureThreshold, setTemperatureThreshold] = useState(45);
  const [voltageThreshold, setVoltageThreshold] = useState(46.5);
  const [healthThreshold, setHealthThreshold] = useState(75);

  // Navigation items for sidebar and header
  const navItems = [
    { name: 'Dashboard', id: 'dashboard' },
    { name: 'Batteries', id: 'batteries' },
    { name: 'Monitoring', id: 'monitoring' },
    { name: 'Alerts', id: 'alerts' },
    { name: 'Reports', id: 'reports' },
    { name: 'Users', id: 'users' },
    { name: 'Settings', id: 'settings' },
  ];

  // Filter batteries based on search term and status filter
  const filteredBatteries = batteries.filter(battery => {
    const matchesSearch = 
      battery.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
      battery.id.toLowerCase().includes(searchTerm.toLowerCase()) ||
      battery.location.toLowerCase().includes(searchTerm.toLowerCase());
    
    const matchesStatus = statusFilter === 'all' || battery.status === statusFilter;
    
    return matchesSearch && matchesStatus;
  }).sort((a, b) => {
    if (sortBy === 'name') {
      return a.name.localeCompare(b.name);
    } else if (sortBy === 'chargeLevel') {
      return b.chargeLevel - a.chargeLevel;
    } else if (sortBy === 'health') {
      return b.health - a.health;
    } else {
      // Sort by status (critical first, then warning, then good)
      const statusOrder = { critical: 0, warning: 1, good: 2, offline: 3 };
      return statusOrder[a.status] - statusOrder[b.status];
    }
  });

  // Get unread alerts count
  const unreadAlertsCount = alerts.filter(alert => !alert.isRead).length;

  // Toggle battery charging
  const toggleCharging = (batteryId: string, e?: React.MouseEvent) => {
    if (e) {
      e.stopPropagation();
    }
    
    setBatteries(prevBatteries => 
      prevBatteries.map(battery => 
        battery.id === batteryId 
          ? { ...battery, isCharging: !battery.isCharging } 
          : battery
      )
    );
  };

  // Mark alert as read
  const markAlertAsRead = (alertId: string) => {
    setAlerts(prevAlerts => 
      prevAlerts.map(alert => 
        alert.id === alertId 
          ? { ...alert, isRead: true } 
          : alert
      )
    );
  };

  // View battery details
  const viewBatteryDetails = (battery: BatteryData) => {
    setSelectedBattery(battery);
    setView('detail');
  };

  // Back to battery list
  const backToList = () => {
    setView('list');
    setSelectedBattery(null);
  };

  // Get system health status
  const getSystemHealth = () => {
    const criticalCount = batteries.filter(b => b.status === 'critical').length;
    const warningCount = batteries.filter(b => b.status === 'warning').length;
    
    if (criticalCount > 0) return 'Critical';
    if (warningCount > 0) return 'Warning';
    return 'Good';
  };

  // Get system health color
  const getSystemHealthColor = () => {
    const health = getSystemHealth();
    if (health === 'Critical') return 'text-red-600';
    if (health === 'Warning') return 'text-amber-600';
    return 'text-green-600';
  };

  // Get average charge level
  const getAverageChargeLevel = () => {
    const total = batteries.reduce((sum, battery) => sum + battery.chargeLevel, 0);
    return Math.round(total / batteries.length);
  };

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
    <div className="min-h-screen bg-gray-100 flex flex-col">
      {/* Mobile menu button - fixed at top left corner for mobile */}
      <button
        className="lg:hidden fixed top-4 left-4 z-50 p-2 rounded-md bg-gray-800 text-white"
        onClick={() => setSidebarOpen(!sidebarOpen)}
      >
        {sidebarOpen ? <X size={24} /> : <Menu size={24} />}
      </button>

      {/* Sidebar */}
      <Sidebar 
        activePage={activePage} 
        setActivePage={setActivePage} 
        sidebarOpen={sidebarOpen} 
        setSidebarOpen={setSidebarOpen}
        unreadAlertsCount={unreadAlertsCount}
      />

      {/* Main Content */}
      <div className="flex-1 flex flex-col lg:ml-64">
        {/* Header - fixed at top */}
        <div className="sticky top-0 z-10">
          <Header 
            activePage={activePage} 
            searchTerm={searchTerm} 
            setSearchTerm={setSearchTerm} 
            unreadAlertsCount={unreadAlertsCount}
            navItems={navItems}
          />
        </div>

        {/* Main Content Area */}
        <main className="flex-1 p-4 sm:p-6 lg:p-8 pb-16">
          {activePage === 'dashboard' && (
            <DashboardPage 
              batteries={batteries}
              alerts={alerts}
              setActivePage={setActivePage}
              setStatusFilter={setStatusFilter}
              getSystemHealth={getSystemHealth}
              getSystemHealthColor={getSystemHealthColor}
              getAverageChargeLevel={getAverageChargeLevel}
              getAlertIcon={getAlertIcon}
            />
          )}

          {activePage === 'batteries' && (
            <BatteriesPage 
              batteries={batteries}
              filteredBatteries={filteredBatteries}
              selectedBattery={selectedBattery}
              statusFilter={statusFilter}
              setStatusFilter={setStatusFilter}
              sortBy={sortBy}
              setSortBy={setSortBy}
              view={view}
              toggleCharging={toggleCharging}
              viewBatteryDetails={viewBatteryDetails}
              backToList={backToList}
              getSystemHealth={getSystemHealth}
              getSystemHealthColor={getSystemHealthColor}
              getAverageChargeLevel={getAverageChargeLevel}
              voltageThreshold={voltageThreshold}
            />
          )}

          {activePage === 'alerts' && (
            <AlertsPage 
              alerts={alerts}
              batteries={batteries}
              markAlertAsRead={markAlertAsRead}
              viewBatteryDetails={viewBatteryDetails}
              setActivePage={setActivePage}
            />
          )}

          {activePage === 'reports' && (
            <ReportsPage />
          )}

          {activePage === 'users' && (
            <UsersPage users={users} />
          )}

          {activePage === 'settings' && (
            <SettingsPage 
              temperatureThreshold={temperatureThreshold}
              setTemperatureThreshold={setTemperatureThreshold}
              voltageThreshold={voltageThreshold}
              setVoltageThreshold={setVoltageThreshold}
              healthThreshold={healthThreshold}
              setHealthThreshold={setHealthThreshold}
            />
          )}
        </main>

        {/* Footer */}
        <Footer />
      </div>
    </div>
  );
}

export default App;