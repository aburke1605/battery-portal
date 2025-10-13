import { useCallback, useEffect, useRef, useState } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import PageMeta from "../../components/common/PageMeta";
import { BatteryData, parseDataOnESP32 } from '../../types';
import BatteryDetail from './BatteryDetail';
import apiConfig from '../../apiConfig';
import { useAuth } from '../../auth/AuthContext';
import { createMessage, fetchBatteryData, useWebSocket } from '../../hooks/useWebSocket';
import { generate_random_string } from '../../utils/helpers';

interface BatteriesPageProps {
    isFromESP32?: boolean;
}

export default function BatteryPage({ isFromESP32 = false }: BatteriesPageProps) {

    // Handle auth_token
    const { getAuthToken } = useAuth();

    let ws_url = apiConfig.WEBSOCKET_BROWSER;
    let queryString = window.location.search;
    if (!isFromESP32) {
        const hash = window.location.hash;  // e.g., "#/battery-detail?esp_id=BMS_02"
        queryString = hash.split('?')[1];  // Extract "esp_id=BMS_02"
    }
    const urlParams = new URLSearchParams(queryString);
    
    const esp_id = Number(urlParams.get('esp_id'));

    const ws_session_browser_id = useRef(generate_random_string(32));
    ws_url = isFromESP32 ? ws_url += "?auth_token=" + getAuthToken() : ws_url += "?browser_id=" + ws_session_browser_id.current + "&esp_id=" + esp_id;

    const [battery, setBatteryData] = useState<BatteryData | null>(null);
    const [voltageThreshold] = useState(48);

    const sleep = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

    if (!isFromESP32) {
        // get fall-back data from DB
        useEffect(() => {
            const loadBattery = async () => {
                const esp = await fetchBatteryData(esp_id);
                if (esp !== null) setBatteryData(esp);
            };

            loadBattery();
        }, []);
    }


    // get fresh data from WebSocket when it connects
    const handleMessage = useCallback(async (data: any) => {
        if (isFromESP32 && data.content) {
            const parsed = parseDataOnESP32(data.content);
            setBatteryData(parsed);
        } else if (data.esp_id === esp_id && data.browser_id === ws_session_browser_id.current) {
            if (data.type === "status_update") {
                const esp = await fetchBatteryData(esp_id);
                if (esp !== null) setBatteryData(esp);
            }
        }
    }, [esp_id, fetchBatteryData]);
    const { sendMessage } = useWebSocket({
        url: ws_url,
        onMessage: handleMessage,
    });

    const sendBatteryUpdate = async (updates: Partial<BatteryData>) => {
        sendMessage(createMessage("change-settings", updates, esp_id));
        if (updates.new_esp_id && updates.new_esp_id !== esp_id) {
            await sleep(5000);
            !isFromESP32 ?
                window.location.replace(`/#/battery-detail?esp_id=${updates.new_esp_id}`)
            :
                window.location.href = `/esp32?esp_id=${updates.new_esp_id}#/`
            ;
        }
    };

    const sendWiFiConnect = (ssid: string, password: string, auto_connect: boolean) =>
        sendMessage(createMessage("connect-wifi", { ssid, password, auto_connect }, esp_id));

    const sendUnseal = () =>
        sendMessage(createMessage("unseal-bms", {}, esp_id));

    const sendReset = () =>
        sendMessage(createMessage("reset-bms", {}, esp_id));

    const updateRequest = async () => {
        const esp = await fetchBatteryData(esp_id);
        if (esp !== null) setBatteryData(esp);
    }

    return (
        <div>
            { 
                !isFromESP32 ? 
                <PageMeta
                    title="Battery Dashboard"
                    description="Battery Detail Page"
                /> : null
            }
            {
                !isFromESP32 ? 
                <PageBreadcrumb pageTitle="Battery Detail" />
                : null
            }
            <div className="space-y-6">
                {battery ? (
                    <BatteryDetail 
                        battery={battery}
                        voltageThreshold={voltageThreshold}
                        sendBatteryUpdate={sendBatteryUpdate} // pass function to BatteryDetail
                        sendWiFiConnect={sendWiFiConnect}
                        sendUnseal={sendUnseal}
                        sendReset={sendReset}
                        isFromESP32={isFromESP32}
                        updateRequest={updateRequest}
                    />
                ) : (
                    <p> Connecting to battery... </p> // fallback UI
                )}
            </div>
        </div>
    );
}
