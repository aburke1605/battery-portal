import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import PageMeta from "../../components/common/PageMeta";
import { ChevronDown, ChevronLeft, ChevronRight } from 'lucide-react';
import { BatteryData } from '../../types';
import BatteryCard from './BatteryCard';
import apiConfig from '../../apiConfig';


export default function BatteriesPage() {
  const [statusFilter, setStatusFilter] = useState('all');
  const [sortBy, setSortBy] = useState('status');
  const [batteries, setBatteries] = useState<BatteryData[]>([]);
  const [searchTerm] = useState('');

  // Get data from Webscocket
  useEffect(() => {
    const ws = new WebSocket(apiConfig.WEBSOCKET_BROWSER);
  
    ws.onopen = () => {
      console.log('WebSocket connected');
    };
  
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        if (data) {
          console.log('Parsed ESPs:', data);
          const batteryArray: BatteryData[] = Object.values(data).map((battery: any) => ({
            id: battery?.name || "id",
            name: battery?.name || "name",
            charge: battery?.charge  || 0,
            voltage: battery?.voltage.toFixed(1) || 0,
            current: battery?.current.toFixed(1) || 0,
            temperature: battery?.temperature.toFixed(1) || 0,
            BL: battery?.BL || 0,
            BH: battery?.BH || 0,
            CITL: battery?.CITL || 0,
            CITH: battery?.CITH || 0,
            IP: battery?.IP || "xxx.xxx.xxx.xxx",
            location: "Unknown",
            health: 100,
            isCharging: false,
            status: "good",
            lastMaintenance: "2025-03-15",
            type: "Lithium-Ion",
            capacity: 100,
            cycleCount: 124,
            timestamp: Date.now(),
          }));
  
          console.log('Parsed batteries:', batteryArray);
          setBatteries(batteryArray);
        }
      } catch (err) {
        console.error('Failed to parse WebSocket message', err);
      }
    };
  
    ws.onclose = () => {
      console.log('WebSocket disconnected');
    };
  
    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
    };
  
    return () => {
      ws.close();
    };
  }, []);
  
  
  

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

    const navigate = useNavigate();
    // View battery details
    const viewBatteryDetails = (battery: BatteryData) => {
      navigate(`/battery-detail?id=${battery.id}`);
    };


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
      return b.charge - a.charge;
    } else if (sortBy === 'health') {
      return b.health - a.health;
    } else {
      // Sort by status (critical first, then warning, then good)
      const statusOrder = { critical: 0, warning: 1, good: 2, offline: 3 };
      return statusOrder[a.status] - statusOrder[b.status];
    }
  });

  const [currentPage, setCurrentPage] = useState(1);
  const itemsPerPage = 6;
  // Calculate pagination
  const totalPages = Math.ceil(filteredBatteries.length / itemsPerPage);
  // const paginatedBatteries = useMemo(() => {
  //   const startIndex = (currentPage - 1) * itemsPerPage;
  //   return filteredBatteries.slice(startIndex, startIndex + itemsPerPage);
  // }, [filteredBatteries, currentPage]);

  // Page navigation
  const goToPage = (page: number) => {
    setCurrentPage(Math.min(Math.max(1, page), totalPages));
  };

  // Generate page numbers
  const getPageNumbers = () => {
    const pages = [];
    const maxVisiblePages = 5;
    let startPage = Math.max(1, currentPage - Math.floor(maxVisiblePages / 2));
    let endPage = Math.min(totalPages, startPage + maxVisiblePages - 1);

    if (endPage - startPage + 1 < maxVisiblePages) {
      startPage = Math.max(1, endPage - maxVisiblePages + 1);
    }

    for (let i = startPage; i <= endPage; i++) {
      pages.push(i);
    }
    return pages;
  };



  return (
    <div>
      <PageMeta
        title="React.js Chart Dashboard | TailAdmin - React.js Admin Dashboard Template"
        description="This is React.js Chart Dashboard page for TailAdmin - React.js Tailwind CSS Admin Dashboard Template"
      />
      <PageBreadcrumb pageTitle="Batteries" />
      <div className="space-y-6">
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

        {/* Pagination */}
        <div className="flex items-center justify-between border-t border-gray-200 bg-white px-4 py-3 sm:px-6 rounded-lg shadow-sm">
          <div className="flex flex-1 justify-between sm:hidden">
            <button
              onClick={() => goToPage(currentPage - 1)}
              disabled={currentPage === 1}
              className={`relative inline-flex items-center rounded-md px-4 py-2 text-sm font-medium ${
                currentPage === 1
                  ? 'bg-gray-100 text-gray-400 cursor-not-allowed'
                  : 'bg-white text-gray-700 hover:bg-gray-50'
              } border border-gray-300`}
            >
              Previous
            </button>
            <button
              onClick={() => goToPage(currentPage + 1)}
              disabled={currentPage === totalPages}
              className={`relative ml-3 inline-flex items-center rounded-md px-4 py-2 text-sm font-medium ${
                currentPage === totalPages
                  ? 'bg-gray-100 text-gray-400 cursor-not-allowed'
                  : 'bg-white text-gray-700 hover:bg-gray-50'
              } border border-gray-300`}
            >
              Next
            </button>
          </div>
          <div className="hidden sm:flex sm:flex-1 sm:items-center sm:justify-between">
            <div>
              <p className="text-sm text-gray-700">
                Showing <span className="font-medium">{(currentPage - 1) * itemsPerPage + 1}</span> to{' '}
                <span className="font-medium">
                  {Math.min(currentPage * itemsPerPage, filteredBatteries.length)}
                </span>{' '}
                of <span className="font-medium">{filteredBatteries.length}</span> batteries
              </p>
            </div>
            <div>
              <nav className="isolate inline-flex -space-x-px rounded-md shadow-sm" aria-label="Pagination">
                <button
                  onClick={() => goToPage(currentPage - 1)}
                  disabled={currentPage === 1}
                  className={`relative inline-flex items-center rounded-l-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:z-20 focus:outline-offset-0 ${
                    currentPage === 1 ? 'cursor-not-allowed' : 'hover:text-gray-700'
                  }`}
                >
                  <span className="sr-only">Previous</span>
                  <ChevronLeft className="h-5 w-5" aria-hidden="true" />
                </button>
                {getPageNumbers().map((page) => (
                  <button
                    key={page}
                    onClick={() => goToPage(page)}
                    className={`relative inline-flex items-center px-4 py-2 text-sm font-semibold ${
                      currentPage === page
                        ? 'z-10 bg-blue-600 text-white focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600'
                        : 'text-gray-900 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:outline-offset-0'
                    }`}
                  >
                    {page}
                  </button>
                ))}
                <button
                  onClick={() => goToPage(currentPage + 1)}
                  disabled={currentPage === totalPages}
                  className={`relative inline-flex items-center rounded-r-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:z-20 focus:outline-offset-0 ${
                    currentPage === totalPages ? 'cursor-not-allowed' : 'hover:text-gray-700'
                  }`}
                >
                  <span className="sr-only">Next</span>
                  <ChevronRight className="h-5 w-5" aria-hidden="true" />
                </button>
              </nav>
            </div>
          </div>
        </div>

      </div>
    </div>
  );
}