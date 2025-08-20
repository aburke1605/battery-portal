import { useCallback, useEffect, useState } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import PageMeta from "../../components/common/PageMeta";
import { BatteryData, parseBatteryData } from '../../types';
import BatteryDetail from './BatteryDetail';
import apiConfig from '../../apiConfig';
import { useAuth } from '../../auth/AuthContext';
import { createMessage, useWebSocket } from '../../hooks/useWebSocket';

interface BatteriesPageProps {
    isFromEsp32?: boolean;
}

export default function BatteryPage({ isFromEsp32 = false }: BatteriesPageProps) {

    // Handle auth_token
    const { getAuthToken } = useAuth();

    let ws_url = apiConfig.WEBSOCKET_BROWSER;
    let queryString = window.location.search;
    if (!isFromEsp32) {
        const hash = window.location.hash;  // e.g., "#/battery-detail?esp_id=BMS_02"
        queryString = hash.split('?')[1];  // Extract "esp_id=BMS_02"
    }
    const urlParams = new URLSearchParams(queryString);
    
    let esp_id = urlParams.get('esp_id');
    if (esp_id == null) esp_id = "empty";

    ws_url = isFromEsp32 ? ws_url += "?auth_token=" + getAuthToken() : ws_url += "?esp_id=" + esp_id;

    const [batteryItem, setSelectedBattery] = useState<BatteryData | null>(null);
    const [voltageThreshold] = useState(46.5);

    const sleep = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

    if (!isFromEsp32) {
        // get fall-back data from DB
        // TODO:
        // make this a FC as its used a few times
        useEffect(() => {
            const fetchBatteryData = async () => {
                try {
                const response = await fetch('/api/battery/data?esp_id='+esp_id);
                const data = await response.json();
                const parsed = parseBatteryData(esp_id, data, false);
                parsed.status = "offline";
                setSelectedBattery(parsed);
                } catch (error) {
                console.error('Error fetching battery data:', error)
                } finally {
                //setLoading(false)
                }
            }

            fetchBatteryData()
            const interval = setInterval(fetchBatteryData, 10000)
            return () => {
                clearInterval(interval)
            }
        }, [])
    }


    // get fresh data from WebSocket when it connects
    const handleMessage = useCallback((data: any) => {
        const battery = data[esp_id];
        const parsed = parseBatteryData(esp_id, battery, true);
        setSelectedBattery(parsed);
    }, [esp_id]);
    const { sendMessage } = useWebSocket({
        url: ws_url,
        onMessage: handleMessage,
    });

    const sendBatteryUpdate = async (updates: Partial<BatteryData>) => {
        sendMessage(createMessage("change-settings", updates, esp_id));
        if (updates.new_esp_id && updates.new_esp_id !== esp_id) {
            await sleep(5000);
            const prefix = window.location.protocol === 'https:' ? 'battery-detail' : 'esp32';
            window.location.href = `/${prefix}?esp_id=${updates.new_esp_id}#/`;
        }
    };

    const sendWiFiConnect = (username: string, password: string, eduroam: boolean) =>
        sendMessage(createMessage("connect-wifi", { username, password, eduroam }, esp_id));

    const sendUnseal = () =>
        sendMessage(createMessage("unseal-bms", {}, esp_id));

    const sendReset = () =>
        sendMessage(createMessage("reset-bms", {}, esp_id));

    return (
        <div>
            { 
                !isFromEsp32 ? 
                <PageMeta
                    title="Battery Dashboard"
                    description="Battery Detail Page"
                /> : null
            }
            {
                !isFromEsp32 ? 
                <PageBreadcrumb pageTitle="Battery Detail" />
                : null
            }
            <div className="space-y-6">
                {batteryItem ? (
                    <BatteryDetail 
                        battery={batteryItem}
                        voltageThreshold={voltageThreshold}
                        sendBatteryUpdate={sendBatteryUpdate} // pass function to BatteryDetail
                        sendWiFiConnect={sendWiFiConnect}
                        sendUnseal={sendUnseal}
                        sendReset={sendReset}
                    />
                ) : (
                    <p> Connecting to battery... </p> // fallback UI
                )}
            </div>
        </div>
    );
}
