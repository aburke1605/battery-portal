import { useState, useEffect, useRef, useCallback } from 'react'
import { ChevronDown } from 'lucide-react';
import { BatteryInfoData, BatteryData } from '../../types';
import BatteryCard from './BatteryCard';
import { useNavigate } from 'react-router-dom';
import apiConfig from '../../apiConfig';
import { generate_random_string } from '../../utils/helpers';
import { fetchBatteryData, useWebSocket } from '../../hooks/useWebSocket';
import "ol/ol.css";
import Map from "ol/Map";
import View from "ol/View";
import TileLayer from "ol/layer/Tile";
import VectorLayer from "ol/layer/Vector";
import VectorSource from "ol/source/Vector";
import OSM from "ol/source/OSM";
import Feature from "ol/Feature";
import Point from "ol/geom/Point";
import { Icon, Style } from "ol/style";
import { fromLonLat } from 'ol/proj';
import Overlay from 'ol/Overlay';

export default function BatteryPage() {
  const itemsPerPage = 4
  const [currentPage, setCurrentPage] = useState(1)
  const [viewMode, setViewMode] = useState<'grid' | 'map'>('grid')

  const mapRef = useRef<HTMLDivElement>(null)
  const mapInstance = useRef<Map | null>(null);
  const markerLayer = useRef<VectorLayer<VectorSource> | null>(null);
  const overlayRef = useRef<Overlay | null>(null);
  const [selectedESPID, setSelectedESPID] = useState<number | null>(null);

  const [batteries, setBatteryData] = useState<BatteryData[]>([])
  let startIndex = (currentPage - 1) * itemsPerPage
  let endIndex = Math.min(startIndex + itemsPerPage, batteries.length)
  let totalPages = Math.ceil(batteries.length / itemsPerPage)

  const [currentBatteries, setCurrentBatteries] = useState<BatteryData[]>([]);
  useEffect(() => {
    startIndex = (currentPage - 1) * itemsPerPage;
    endIndex = Math.min(startIndex + itemsPerPage, batteries.length);
    totalPages = Math.ceil(batteries.length / itemsPerPage)
    setCurrentBatteries(batteries.slice(startIndex, endIndex));
  }, [batteries, currentPage, itemsPerPage]); // update as batteries (and other variables) do
  
  

  const [statusFilter, setStatusFilter] = useState('all');
  const [sortBy, setSortBy] = useState('status');


  // initial fetch
  useEffect(() => {
    const loadBatteries = async () => {
      const esps = await fetchBatteryData(-999);
      if (esps !== null) setBatteryData(esps);
    }

    loadBatteries();
  }, [fetchBatteryData])

  // get status updates from backend through websocket
  const ws_session_browser_id = useRef(generate_random_string(32));
  const ws_url = `${apiConfig.WEBSOCKET_BROWSER}?browser_id=${ws_session_browser_id.current}&esp_id=-999`;
  // fetch from database on message receipt
  const handleMessage = useCallback(async (data: any) => {
    if (data.esp_id === -999 && data.browser_id === ws_session_browser_id.current) {
      if (data.type === "status_update"){
        const esps = await fetchBatteryData(-999);
        if (esps !== null) setBatteryData(esps);
      }
    }
  }, [fetchBatteryData]);
  useWebSocket({
      url: ws_url,
      onMessage: handleMessage,
  });


  // initialise map layers (once!)
  useEffect(() => {
    if (mapRef.current && !mapInstance.current) {
      // create empty marker layer
      markerLayer.current = new VectorLayer({ source: new VectorSource() });

      // create overlay
      const popupEl = document.getElementById("popup")!;
      overlayRef.current = new Overlay({
        element: popupEl,
        positioning: "bottom-center",
        stopEvent: true,
      });

      // create map
      const map = new Map({
        target: mapRef.current,
        layers: [
          new TileLayer({ source: new OSM() }),
          markerLayer.current,
        ],
        view: new View({
          center: fromLonLat([-0.1276, 51.5074]),
          zoom: 5,
        }),
        overlays: [overlayRef.current],
      });
      mapInstance.current = map;

      // click handler
      const handleClick = (evt: any) => {
        let found = false;
        map.forEachFeatureAtPixel(evt.pixel, (feature) => {
          found = true;
          const battery = feature.get("batteryData") as BatteryData;
          if (!battery) return;
          setSelectedESPID(battery.esp_id);
          overlayRef.current!.setPosition((feature.getGeometry() as Point).getCoordinates());
        });

        if (!found) {
          setSelectedESPID(null);
          overlayRef.current!.setPosition(undefined); // hides popup
        }
      };

      // hover cursor handler
      const handlePointerMove = (evt: any) => {
        const hit = map.hasFeatureAtPixel(evt.pixel);
        map.getTargetElement().style.cursor = hit ? "pointer" : "";
      };

      map.on("click", handleClick);
      map.on("pointermove", handlePointerMove);

      return () => {
        map.un("click", handleClick);
        map.un("pointermove", handlePointerMove);
      };
    }
  }, []);


  // dynamically update marker layer on top of map
  useEffect(() => {
    if (!markerLayer.current) return;

    // create marker feature
    const newMarkers = currentBatteries.map((battery: BatteryData) => {
      const marker = new Feature({
        geometry: new Point(fromLonLat([battery.lon, battery.lat])),
        batteryData: battery,
      });
      marker.setStyle(
        new Style({
          image: new Icon({
            src: "battery.png",
            scale: 0.15,
            color: battery.live_websocket?'#10B981':'#6B7280',
            rotation: -Math.PI / 2,
          }),
        })
      );
      return marker;
    })

    markerLayer.current.getSource()?.clear();
    markerLayer.current.getSource()?.addFeatures(newMarkers);

  }, [currentBatteries]);


  const handlePageChange = (page: number) => {
    setCurrentPage(page)
    window.scrollTo({ top: 0, behavior: 'smooth' })
  }

  const navigate = useNavigate();
  // View battery details
  const viewBatteryDetails = (battery: BatteryInfoData) => {
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

        <div className={
          viewMode==="grid"?"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-2 xl:grid-cols-3 gap-6"
          :"hidden"
        }>
          {currentBatteries.map((battery: BatteryData) => (
            <BatteryCard
              battery={battery}
              viewBatteryDetails={viewBatteryDetails}
            />
          ))}
        </div>

        <div className={
          viewMode==="map"?"bg-white rounded-lg shadow overflow-hidden w-full"
          :"hidden"
        }>
          <div ref={mapRef} className="w-full h-[700px]" />
          <div
            id="popup"
            className="ol-popup"
            style={{
              background: "white",
              opacity: 1,
              borderRadius: "8px",
              boxShadow: "0 2px 8px rgba(0,0,0,0.3)",
              padding: "8px",
            }}
          >
            {selectedESPID && (() => {
                const selectedBattery = batteries.find(b => b.esp_id === selectedESPID);
                if (!selectedBattery) return null;
                return (
                  <div className="p-3 min-w-[200px]">
                    <h3 className="font-semibold text-lg">{selectedBattery.esp_id}</h3>
                    <p className="text-sm text-gray-600 mt-1">
                      Status: {selectedBattery.live_websocket ? "online" : "offline"}
                    </p>
                    <div className="mt-2 text-sm">
                      <p>Charge: {selectedBattery.Q}</p>
                      <p>Health: {selectedBattery.H}</p>
                    </div>
                    <div className="mt-3 pt-3 border-t border-gray-200">
                      <button
                        onClick={() => viewBatteryDetails(selectedBattery)}
                        className="inline-block w-full text-center px-4 py-2 bg-blue-600 text-white text-sm font-medium rounded hover:bg-blue-700 transition-colors"
                      >
                        View Details
                      </button>
                    </div>
                  </div>
                );
              })()}
          </div>
        </div>

      {/* Show pagination only in grid view */}
      {viewMode === 'grid' && (
        <div className="mt-8 flex flex-col items-center gap-4">
          <div className="flex items-center justify-between w-full max-w-2xl">
            <div className="text-sm text-gray-600">
              Showing {startIndex + 1} to {endIndex} of {batteries.length} entries
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