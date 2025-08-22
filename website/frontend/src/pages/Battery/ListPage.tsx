import { useState, useEffect, useRef, useCallback } from 'react'
import { ChevronDown } from 'lucide-react';
import { BatteryData } from '../../types';
import BatteryCard from './BatteryCard';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';
import apiConfig from '../../apiConfig';
import { generate_random_string } from '../../utils/helpers';
import { useWebSocket } from '../../hooks/useWebSocket';

// Add this interface for map markers
interface MapMarker {
  id: string
  position: {
    lat: number
    lng: number
  }
  title: string
  status: 'online' | 'offline'
}

// Update the map markers to be more spread across the UK
const mapMarkers: MapMarker[] = []

export default function BatteryPage() {
  const itemsPerPage = 4
  const [currentPage, setCurrentPage] = useState(1)
  const [viewMode, setViewMode] = useState<'grid' | 'map'>('grid')
  const mapRef = useRef<HTMLDivElement>(null)
  const [map, setMap] = useState<google.maps.Map | null>(null)
  const [markers, setMarkers] = useState<google.maps.Marker[]>([])
  
  const [batteryData, setBatteryData] = useState<BatteryData[]>([])
  const totalItems = batteryData.length
  const startIndex = (currentPage - 1) * itemsPerPage
  const endIndex = Math.min(startIndex + itemsPerPage, totalItems)
  const currentBatteries = batteryData.slice(startIndex, endIndex)
  
  const totalPages = Math.ceil(totalItems / itemsPerPage)
  

  const [statusFilter, setStatusFilter] = useState('all');
  const [sortBy, setSortBy] = useState('status');

  // TODO:
  // make this a FC as its used a few times
  const fetchBatteryData = useCallback(async () => {
    try {
      const response = await axios.get(`${apiConfig.DB_INFO_API}`);
      setBatteryData(response.data);
    } catch(error) {
      console.error("Error fetching battery data:", error);
    } finally {
      // setLoading(false);
    }
  }, []);

  // initial fetch
  useEffect(() => {
    fetchBatteryData();
  }, [fetchBatteryData])

  // get status updates from backend through websocket
  const ws_session_browser_id = useRef(generate_random_string(32));
  const ws_url = `${apiConfig.WEBSOCKET_BROWSER}?browser_id=${ws_session_browser_id.current}&esp_id=NONE`;
  // fetch from database on message receipt
  const handleMessage = useCallback((data: any) => {
    if (data.esp_id === "NONE" && data.browser_id === ws_session_browser_id.current) {
      if (data.type === "status_update"){
        fetchBatteryData();
      }
    }
  }, [fetchBatteryData]);
  useWebSocket({
      url: ws_url,
      onMessage: handleMessage,
  });


  // Initialize Google Maps
  useEffect(() => {
    if (viewMode === 'map' && mapRef.current && !map) {
      const script = document.createElement('script')
      script.src = `https://maps.googleapis.com/maps/api/js?key=AIzaSyDnwT87aH7CyWG6c7gSNxgA7lfclPOVpvQ`
      script.async = true
      script.defer = true
      script.onload = initializeMap
      document.head.appendChild(script)
    }
  }, [viewMode, map])

  const initializeMap = () => {
    if (mapRef.current) {
      const ukCenter = { lat: 54.5, lng: -2.0 }
      const newMap = new google.maps.Map(mapRef.current, {
        center: ukCenter,
        zoom: 6,
       
      })

      setMap(newMap)
      addMarkers(newMap)
    }
  }

  const getBatteryIcon = (status: string) => {
    const color = getMarkerColor(status)
    return {
      path: 'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm-1-13h2v6h-2zm0 8h2v2h-2z',
      fillColor: color,
      fillOpacity: 1,
      strokeColor: '#ffffff',
      strokeWeight: 2,
      scale: 1.2,
      anchor: new google.maps.Point(12, 12)
    }
  }

  const addMarkers = (map: google.maps.Map) => {
    const newMarkers = mapMarkers.map(marker => {
      const markerObj = new google.maps.Marker({
        position: marker.position,
        map,
        title: marker.title,
        icon: getBatteryIcon(marker.status)
      })

      // Add click listener with enhanced info window
      markerObj.addListener('click', () => {
        const infoWindow = new google.maps.InfoWindow({
          content: `
            <div class="p-3 min-w-[200px]">
              <h3 class="font-semibold text-lg">${marker.title}</h3>
              <p class="text-sm text-gray-600 mt-1">Status: ${marker.status}</p>
              <div class="mt-2 text-sm">
                <p>Capacity: ${marker.id === 'b4' ? '85%' : '90%'}</p>
                <p>Connected Units: ${marker.id === 'b4' ? '12' : '8'}</p>
                <p>Last Maintenance: ${marker.id === 'b4' ? '2 days ago' : '1 week ago'}</p>
              </div>
              <div class="mt-3 pt-3 border-t border-gray-200">
                <a 
                  href="/battery/${marker.id}" 
                  class="inline-block w-full text-center px-4 py-2 bg-blue-600 text-white text-sm font-medium rounded hover:bg-blue-700 transition-colors"
                  onclick="window.location.href='/battery/${marker.id}'; return false;"
                >
                  View Details
                </a>
              </div>
            </div>
          `
        })
        infoWindow.open(map, markerObj)
      })

      return markerObj
    })

    setMarkers(newMarkers)
  }

  const getMarkerColor = (status: string) => {
    switch (status) {
      case 'online':
        return '#10B981' // green-500
      case 'offline':
        return '#6B7280' // gray-500
      default:
        return '#6B7280' // gray-500
    }
  }

  // Cleanup markers when component unmounts
  useEffect(() => {
    return () => {
      markers.forEach(marker => marker.setMap(null))
    }
  }, [markers])

  const handlePageChange = (page: number) => {
    setCurrentPage(page)
    window.scrollTo({ top: 0, behavior: 'smooth' })
  }

  const navigate = useNavigate();
  // View battery details
  const viewBatteryDetails = (battery: BatteryData) => {
    navigate(`/battery-detail?esp_id=${battery.esp_id}`);
  };
  

  return (
    <main className="container mx-auto px-4 py-8">
      <div className="flex justify-between items-center mb-6">
     
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
            
            <div className="flex items-center space-x-2" style={{ display: 'none' }}>
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
        </div>
         <button 
          onClick={() => setViewMode(viewMode === 'grid' ? 'map' : 'grid')}
          className="flex space-x-2 items-center gap-2 px-4 py-2 bg-white rounded-lg border border-gray-200 shadow-sm hover:bg-gray-50 transition-all duration-200 ease-in-out"
        >
          <span className="text-sm font-medium text-gray-700">
            {viewMode === 'grid' ? 'Map' : 'Grid'}
          </span>
          <div className="w-5 h-5 flex items-center justify-center">
            <svg
              className={`w-4 h-4 text-gray-600 transition-transform duration-200 ${viewMode === 'grid' ? 'rotate-180' : ''}`}
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              {viewMode === 'map' ? (
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z"
                />
              ) : (
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M9 20l-5.447-2.724A1 1 0 013 16.382V5.618a1 1 0 011.447-.894L9 7m0 13l6-3m-6 3V7m6 10l4.553 2.276A1 1 0 0021 18.382V7.618a1 1 0 00-.553-.894L15 4m0 13V4m0 0L9 7"
                />
              )}
            </svg>
          </div>
        </button>
      </div>

      {viewMode === 'grid' ? (
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-2 xl:grid-cols-3 gap-6">
          {currentBatteries.map((battery: BatteryData) => (
            <BatteryCard
            battery={battery}
            viewBatteryDetails={viewBatteryDetails}
            />
          ))}
        </div>
      ) : (
        <div className="bg-white rounded-lg shadow overflow-hidden w-full">
          <div ref={mapRef} className="w-full h-[700px]" />
        </div>
      )}

      {/* Show pagination only in grid view */}
      {viewMode === 'grid' && (
        <div className="mt-8 flex flex-col items-center gap-4">
          <div className="flex items-center justify-between w-full max-w-2xl">
            <div className="text-sm text-gray-600">
              Showing {startIndex + 1} to {endIndex} of {totalItems} entries
            </div>
            
            <div className="flex items-center gap-2">
              <button
                onClick={() => handlePageChange(1)}
                disabled={currentPage === 1}
                className="px-3 py-1 rounded border border-gray-300 disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-50"
              >
                First
              </button>
              
              <button
                onClick={() => handlePageChange(currentPage - 1)}
                disabled={currentPage === 1}
                className="px-3 py-1 rounded border border-gray-300 disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-50"
              >
                Previous
              </button>

              <div className="flex gap-1">
                {Array.from({ length: totalPages }, (_, i) => i + 1).map((page) => (
                  <button
                    key={page}
                    onClick={() => handlePageChange(page)}
                    className={`w-8 h-8 rounded ${
                      currentPage === page
                        ? 'bg-blue-600 text-white'
                        : 'border border-gray-300 hover:bg-gray-50'
                    }`}
                  >
                    {page}
                  </button>
                ))}
              </div>

              <button
                onClick={() => handlePageChange(currentPage + 1)}
                disabled={currentPage === totalPages}
                className="px-3 py-1 rounded border border-gray-300 disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-50"
              >
                Next
              </button>

              <button
                onClick={() => handlePageChange(totalPages)}
                disabled={currentPage === totalPages}
                className="px-3 py-1 rounded border border-gray-300 disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-50"
              >
                Last
              </button>
            </div>

            <div className="text-sm text-gray-600">
              Page {currentPage} of {totalPages}
            </div>
          </div>
        </div>
      )}
    </main>
  )
}

const styles = `
  .custom-scrollbar::-webkit-scrollbar {
    width: 4px;
  }

  .custom-scrollbar::-webkit-scrollbar-track {
    background: #f1f1f1;
    border-radius: 2px;
  }

  .custom-scrollbar::-webkit-scrollbar-thumb {
    background: #cbd5e1;
    border-radius: 2px;
  }

  .custom-scrollbar::-webkit-scrollbar-thumb:hover {
    background: #94a3b8;
  }
`

if (typeof document !== 'undefined') {
  const styleSheet = document.createElement('style')
  styleSheet.textContent = styles
  document.head.appendChild(styleSheet)
} 