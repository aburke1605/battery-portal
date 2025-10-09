import { useEffect, useRef, useCallback } from "react";
import { BatteryData, BatteryInfoData } from "../types";
import axios from "axios";
import apiConfig from "../apiConfig";

export function useWebSocket({
  url,
  onMessage,
  autoConnect = true,
}: {
  url: string;
  onMessage: (data: any) => void;
  autoConnect?: boolean;
}) {
  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    if (!autoConnect) return;

    const socket = new WebSocket(url);
    ws.current = socket;

    socket.onopen = () => console.log("WebSocket connected:", url);
    socket.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        onMessage(data);
      } catch (e) {
        console.error("Invalid JSON from WebSocket", e);
      }
    };
    socket.onclose = () => console.log("WebSocket disconnected:", url);
    socket.onerror = (e) => console.error("WebSocket error:", e);

    return () => socket.close();
  }, [url, autoConnect, onMessage]);

  const sendMessage = useCallback((msg: any) => {
    if (ws.current?.readyState === WebSocket.OPEN) {
      const payload = JSON.stringify(msg);
      ws.current.send(payload);
      console.log("Sent:", payload);
    } else {
      console.warn("WebSocket not connected");
    }
  }, []);

  return { sendMessage };
}



export const createMessage = (
  summary: string,
  data: any,
  esp_id?: number
) => ({
  type: "request",
  content: {
    summary,
    data: esp_id ? { esp_id, ...data } : data,
  },
});


export async function fetchBatteryData(esp_id: number) {
  /*
    fetches data from battery_info table in database
  */
  try {
      const response = await axios.get(`${apiConfig.DB_INFO_API}`);
      const batteries = extendBatteryInfo(response.data);

      // add telemetry data (recusively) to battery info
      const detailed = await Promise.all(batteries.map(appendBatteryData));

      if (esp_id === -999) { // special ID for ListPage
        return detailed;
      } else {
        // extract only one battery data item
        return extractSingleBattery(detailed, esp_id);
      }

  } catch(error) {
      console.error("Error fetching battery data:", error);
      return null;
  }
}

// parsing helper
const extendBatteryInfo = (data: BatteryInfoData[]): BatteryData[] => {
    /*
      contructs BatteryData (which is BatteryInfo + extra from battery_data_<esp_id>) objects,
      initially populating them with data from BatteryInfo inputs
    */
    return data.map((root) => ({
        esp_id: root.esp_id,
        root_id: root.root_id,
        last_updated_time: root.last_updated_time,
        live_websocket: root.live_websocket,
        nodes: root.nodes,

        // fetch these from api in fetchBatteryData later:
        t: 0,
        d: 0,
        lat: 0,
        lon: 0,
        Q: 0,
        H: 0,
        V: 0,
        V1: 0,
        V2: 0,
        V3: 0,
        V4: 0,
        I: 0,
        I1: 0,
        I2: 0,
        I3: 0,
        I4: 0,
        aT: 0,
        cT: 0,
        T1: 0,
        T2: 0,
        T3: 0,
        T4: 0,
        new_esp_id: 0,
        OTC: 0,
        Q_low: 0,
        Q_high: 0,
        I_dschg_max: 0,
        wifi: false
    }));
};

// recursive helper
const appendBatteryData = async (battery: any): Promise<any> => {
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
            const mergedNodes = await Promise.all(battery.nodes.map(appendBatteryData));
            return { ...merged, nodes: mergedNodes };
        }

        return merged;
    } catch {
        // fallback if fetch fails
        if (battery.nodes && battery.nodes.length > 0) {
            const mergedNodes = await Promise.all(battery.nodes.map(appendBatteryData));
            return { ...battery, nodes: mergedNodes };
        }
        return battery;
    }
};

// recursive helper
const extractSingleBattery = (info: BatteryData[]|BatteryInfoData[], esp_id: number): any => {
    /*
        to loop through the json returned at /api/db/info,
        returning the correct object according to esp_id
    */
    for (const esp of info) {
        if (esp.esp_id === esp_id) {
            return esp;
        }
        if (esp.nodes && esp.nodes.length > 0) {
            const found = extractSingleBattery(esp.nodes, esp_id);
            if (found) return found;
        }
    }
    return null;
}
