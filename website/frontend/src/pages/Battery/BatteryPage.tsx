import { useState, useEffect, useRef } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import PageMeta from "../../components/common/PageMeta";
import { BatteryData } from '../../types';
import BatteryDetail from './BatteryDetail';
import apiConfig from '../../apiConfig';

interface BatteriesPageProps {
    isFromEsp32?: boolean;
}

export default function BatteryPage({ isFromEsp32 = false }: BatteriesPageProps) {
      let queryString = window.location.search;
      if (!isFromEsp32) {
          const hash = window.location.hash;  // e.g., "#/battery-detail?esp_id=BMS_02"
          queryString = hash.split('?')[1];  // Extract "esp_id=BMS_02"
    }
    const urlParams = new URLSearchParams(queryString);
    const [esp_id, reset_esp_id] = useState(urlParams.get('esp_id'));

    const [batteryItem, setSelectedBattery] = useState<BatteryData | null>(null);
    //const [setBatteries] = useState<BatteryData[]>(initialBatteries);
    const [voltageThreshold] = useState(46.5);

    // Get data from Webscocket
    const ws = useRef<WebSocket | null>(null);
    useEffect(() => {
       
        ws.current = new WebSocket(apiConfig.WEBSOCKET_BROWSER);

        ws.current.onopen = () => {
          console.log('WebSocket connected');
        };
      
        ws.current.onmessage = (event) => {
            console.log('Received message', event.data);
            if (!esp_id) {
                return;
            }
            try {
                const data = JSON.parse(event.data);
                console.log('Received WebSocket message:', data);
                const battery = data[esp_id];
                const batteryItem: BatteryData = {
                    esp_id: esp_id,
                    new_esp_id: "",
                    charge: battery?.charge  || 0,
                    voltage: battery?.voltage.toFixed(1) || 0,
                    current: battery?.current.toFixed(1) || 0,
                    temperature: battery?.temperature.toFixed(1) || 0,
                    BL: battery?.BL || 0,
                    BH: battery?.BH || 0,
                    CITL: battery?.CITL || 0,
                    CITH: battery?.CITH || 0,
                    CCT: battery?.CCT || 0,
                    DCT: battery?.DCT || 0,
                    IP: battery?.IP || "xxx.xxx.xxx.xxx",
                    isConnected: battery?.connected_to_WiFi || false,
                    location: "Unknown",
                    health: 100,
                    isCharging: false,
                    status: "good",
                    lastMaintenance: "2025-03-15",
                    type: "Lithium-Ion",
                    capacity: 100,
                    cycleCount: 124,
                    timestamp: Date.now(),
                };
                console.log('Found battery:', batteryItem);
                setSelectedBattery(batteryItem || null);
            } catch (err) {
                console.error('Failed to parse WebSocket message', err);
            }
        };
      
        ws.current.onclose = () => {
          console.log('WebSocket disconnected');
        };
      
        ws.current.onerror = (err) => {
          console.error('WebSocket error:', err);
        };
      
        return () => {
          ws.current?.close();
        };
    }, [esp_id]);

    // Function to send updated values to the WebSocket
    const sendBatteryUpdate = (updatedValues: Partial<BatteryData>) => {
        if (ws.current && ws.current.readyState === WebSocket.OPEN) {
            const message = JSON.stringify({
                type: "request",
                content : {
                    summary: "change-settings",
                    data : {
                        esp_id, // Send the battery ID so the server knows which battery to update
                        ...updatedValues, // Send only the changed values
                    },
                },
            });
            ws.current.send(message);
            console.log("Sent update:", message);
            reset_esp_id(updatedValues?.new_esp_id || esp_id);
        } else {
            console.warn("WebSocket not connected, cannot send update.");
        }
    };
    const sendWiFiConnect = (username: string, password: string, eduroam: boolean) => {
        if (ws.current && ws.current.readyState === WebSocket.OPEN) {
            const message = JSON.stringify({
                type: "request",
                content : {
                    summary: "connect-wifi",
                    data : {
                        esp_id,
                        username,
                        password,
                        eduroam,
                    },
                },
            });
            ws.current.send(message);
            console.log("Sent update:", message);
        } else {
            console.warn("WebSocket not connected, cannot send update.");
        }
    };

    // TODO
    const toggleCharging = (batteryId: string, e?: React.MouseEvent) => {
        if (e) {
            e.stopPropagation();
          }
        alert(`Charging toggled for battery ${batteryId}`);
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
                        voltageThreshold={voltageThreshold}
                        sendBatteryUpdate={sendBatteryUpdate} // pass function to BatteryDetail
                        sendWiFiConnect={sendWiFiConnect}
                    />
                ) : (
                    <p>Loading battery data...</p> // fallback UI
                )}
            </div>
        </div>
    );
}
