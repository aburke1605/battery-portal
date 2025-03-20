import { useState, useEffect } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import PageMeta from "../../components/common/PageMeta";
import { BatteryData } from '../../types';
import BatteryDetail from './BatteryDetail';
import apiConfig from '../../apiConfig';

interface BatteriesPageProps {
    isFromEsp32?: boolean;
}

  export default function BatteriesPage({ isFromEsp32 = false }: BatteriesPageProps) {
      let queryString = window.location.search;
      if (!isFromEsp32) {
          const hash = window.location.hash;  // e.g., "#/battery-detail?id=BMS_02"
          queryString = hash.split('?')[1];  // Extract "id=BMS_02"
    }
    const urlParams = new URLSearchParams(queryString);
    const id = urlParams.get('id');

    const [batteryItem, setSelectedBattery] = useState<BatteryData | null>(null);
    //const [setBatteries] = useState<BatteryData[]>(initialBatteries);
    const [voltageThreshold] = useState(46.5);

     // Get data from Webscocket
      useEffect(() => {
        const ws = new WebSocket(apiConfig.WEBSOCKET_BROWSER);

        ws.onopen = () => {
          console.log('WebSocket connected');
        };
      
        ws.onmessage = (event) => {
            console.log('Received message', event.data);
            if (!id) {
                return;
            }
            try {
                const data = JSON.parse(event.data);
                console.log('Received WebSocket message:', data);
                console.log('No state, looking up battery by id...');
                const battery = data[id];
                const batteryItem: BatteryData = {
                    id: battery?.name || "id",
                    name: battery?.name || "name",
                    charge: battery?.charge  || 0,
                    voltage: battery?.voltage.toFixed(1) || 0,
                    current: battery?.current.toFixed(1) || 0,
                    temperature: battery?.temperature.toFixed(1) || 0,
                    IP: battery?.IP || "xxx.xxx.xxx.xxx",
                    location: "Unknown",
                    health: 100,
                    isCharging: false,
                    status: "good",
                    lastMaintenance: "2025-03-15",
                    type: "Lithium-Ion",
                    capacity: 100,
                    cycleCount: 124
                };
                console.log('Found battery:', batteryItem);
                setSelectedBattery(batteryItem || null);
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
    }, [id]);

    // TODO
    const toggleCharging = (batteryId: string, e?: React.MouseEvent) => {
        if (e) {
            e.stopPropagation();
          }
        alert(`Charging toggled for battery ${batteryId}`);
    };

    const toggleLED = async (batteryId: string, e?: React.MouseEvent) => {
        if (e) {
          e.stopPropagation();
        }
      
        try {
          const response = await fetch(apiConfig.TOGGLE_LED + '?esp_id='+batteryId, {
            method: 'POST'
          });
      
          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }
      
          const result = await response.json();
          console.log('LED toggle success:', result);
          alert(`LED toggled for battery ${batteryId}`);
        } catch (error) {
          console.error('Error toggling LED:', error);
          alert(`Failed to toggle LED for battery ${batteryId}`);
        }
    };

    return (
        <div>
            { 
                !isFromEsp32 ? 
                <PageMeta
                    title="React.js Chart Dashboard | TailAdmin"
                    description="Battery Detail Page"
                /> : null
            }
            {
                !isFromEsp32 ? 
                <PageBreadcrumb pageTitle="Batteries Detail" />
                : null
            }
            <div className="space-y-6">
                {batteryItem ? (
                    <BatteryDetail 
                        battery={batteryItem}
                        onToggleCharging={toggleCharging}
                        onToggleLED={toggleLED}
                        voltageThreshold={voltageThreshold}
                    />
                ) : (
                    <p>Loading battery data...</p> // fallback UI
                )}
            </div>
        </div>
    );
}
