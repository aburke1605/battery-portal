import { useCallback, useEffect, useRef, useState } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import PageMeta from "../../components/common/PageMeta";
import { BatteryDataNew, BatteryInfoData, parseBatteryDataNew } from '../../types';
import BatteryDetail from './BatteryDetail';
import apiConfig from '../../apiConfig';
import { useAuth } from '../../auth/AuthContext';
import { createMessage, useWebSocket } from '../../hooks/useWebSocket';
import axios from 'axios';
import { generate_random_string } from '../../utils/helpers';

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

    const ws_session_browser_id = useRef(generate_random_string(32));
    ws_url = isFromEsp32 ? ws_url += "?auth_token=" + getAuthToken() : ws_url += "?browser_id=" + ws_session_browser_id.current + "&esp_id=" + esp_id;

    const [batteryItem, setSelectedBattery] = useState<BatteryDataNew | null>(null);
    const [voltageThreshold] = useState(46.5);

    const sleep = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

    const parseBatteryInfoMESHs = (data: BatteryInfoData[]): BatteryDataNew[] => {
        return data.map((root) => ({
            esp_id: root.esp_id,
            root_id: root.root_id,
            last_updated_time: root.last_updated_time,
            live_websocket: root.live_websocket,
            nodes: root.nodes,

            // fetch these from api in fetchBatteryInfo later:
            t: 0,
            Q: 0,
            H: 0,
            cT: 0,
            V: 0,
            I: 0,
            new_esp_id: "",
            OTC: 0,
            wifi: false
        }));
    };

    const get_sub_info = (info: BatteryDataNew[]|BatteryInfoData[], esp_id: string): any => {
        /*
            to loop through the json returned at /api/db/info,
            returning the correct object according to esp_id
        */
        for (const esp of info) {
            if (esp.esp_id === esp_id) {
                return esp;
            }
            if (esp.nodes && esp.nodes.length > 0) {
                const found = get_sub_info(esp.nodes, esp_id);
                if (found) return found;
            }
        }
        return null;
    }

    // recursive helper
    const addBatteryDataToInfo = async (battery: any): Promise<any> => {
        /*
            to loop through the json returned at /api/db/data,
            merging data defined in BatteryData to the already existing data defined in BatteryInfo
        */
        try {
            const res = await axios.get(`${apiConfig.DB_DATA_API}?esp_id=${battery.esp_id}`);
            // merge root battery with fetched telemetry
            const merged = { ...battery, ...res.data };

            // recurse on children if any
            if (battery.nodes && battery.nodes.length > 0) {
                const mergedNodes = await Promise.all(battery.nodes.map(addBatteryDataToInfo));
                return { ...merged, nodes: mergedNodes };
            }

            return merged;
        } catch {
            // fallback if fetch fails
            if (battery.nodes && battery.nodes.length > 0) {
                const mergedNodes = await Promise.all(battery.nodes.map(addBatteryDataToInfo));
                return { ...battery, nodes: mergedNodes };
            }
            return battery;
        }
    };

    // TODO:
    // make this a FC as its used a few times
    const fetchBatteryInfo = useCallback(async () => {
        try {
            const response = await axios.get(`${apiConfig.DB_INFO_API}`);
            const batteries = parseBatteryInfoMESHs(response.data);

            // add telemetry data (recusively) to battery info
            const detailed = await Promise.all(batteries.map(addBatteryDataToInfo));

            // extract only one battery data item
            const esp = get_sub_info(detailed, esp_id);
            if (esp !== null) setSelectedBattery(esp);

        } catch(error) {
            console.error("Error fetching battery data:", error);
        } finally {
            // setLoading(false);
        }
    }, []);

    if (!isFromEsp32) {
        // get fall-back data from DB
        useEffect(() => {
            fetchBatteryInfo();
        }, [fetchBatteryInfo])
    }


    // get fresh data from WebSocket when it connects
    const handleMessage = useCallback((data: any) => {
        if (isFromEsp32 || (data.esp_id === esp_id && data.browser_id === ws_session_browser_id.current)) {
            if (data.type === "status_update") {
                fetchBatteryInfo();
            } else if (data.content) {
                const parsed = parseBatteryDataNew(data.content);
                setSelectedBattery(parsed);
            }
        }
    }, [esp_id, fetchBatteryInfo]);
    const { sendMessage } = useWebSocket({
        url: ws_url,
        onMessage: handleMessage,
    });

    const sendBatteryUpdate = async (updates: Partial<BatteryDataNew>) => {
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
