import { useState, useEffect, useRef } from 'react';
import { useNavigate } from 'react-router-dom';
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
    const esp_id = urlParams.get('esp_id');

    const [batteryItem, setSelectedBattery] = useState<BatteryData | null>(null);
    //const [setBatteries] = useState<BatteryData[]>(initialBatteries);
    const [voltageThreshold] = useState(46.5);

    const navigate = useNavigate();
    const sleep = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

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
                    health: battery?.health || 0,
                    voltage: battery?.voltage.toFixed(1) || 0,
                    current: battery?.current.toFixed(1) || 0,
                    temperature: battery?.temperature.toFixed(1) || 0,
                    internal_temperature: battery?.internal_temperature.toFixed(1) || 0,
                    BL: battery?.BL || 0,
                    BH: battery?.BH || 0,
                    CITL: battery?.CITL || 0,
                    CITH: battery?.CITH || 0,
                    CCT: battery?.CCT || 0,
                    DCT: battery?.DCT || 0,
                    IP: battery?.IP || "xxx.xxx.xxx.xxx",
                    isConnected: battery?.connected_to_WiFi || false,
                    // location: "Unknown",
                    // isCharging: false,
                    status: "good",
                    // lastMaintenance: "2025-03-15",
                    // type: "Lithium-Ion",
                    // capacity: 100,
                    // cycleCount: 124,
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
    const sendBatteryUpdate = async (updatedValues: Partial<BatteryData>) => {
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
            if (updatedValues.new_esp_id && updatedValues.new_esp_id != esp_id) {
                console.log("ESP ID changed, redirecting in 5s...");
                await sleep(5000);
                const prefix = window.location.protocol == 'https:' ? 'battery-detail' : 'esp32';
                navigate(`/${prefix}?esp_id=${updatedValues.new_esp_id}`);
            }
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
    const sendReset = () => {
        if (ws.current && ws.current.readyState === WebSocket.OPEN) {
            const message = JSON.stringify({
                type: "request",
                content : {
                    summary: "reset-bms",
                    data : {
                        esp_id,
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
    /* const toggleCharging = (batteryId: string, e?: React.MouseEvent) => {
        if (e) {
            e.stopPropagation();
          }
        alert(`Charging toggled for battery ${batteryId}`);
    }; */

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
                        // onToggleCharging={toggleCharging}
                        voltageThreshold={voltageThreshold}
                        sendBatteryUpdate={sendBatteryUpdate} // pass function to BatteryDetail
                        sendWiFiConnect={sendWiFiConnect}
                        sendReset={sendReset}
                    />
                ) : (
                    <p>Loading battery data...</p> // fallback UI
                )}
            </div>
        </div>
    );
}
