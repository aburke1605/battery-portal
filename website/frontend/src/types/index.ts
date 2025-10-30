// Battery Data Types
export interface BatteryInfoData {
  esp_id: number;
  root_id: number|null;
  last_updated_time: string;
  live_websocket: boolean;
  nodes?: BatteryInfoData[];
}

export interface BatteryData extends BatteryInfoData {
  t: number;
  d: number;
  lat: number;
  lon: number;
  Q: number
  H: number;
  V: number;
  V1: number;
  V2: number;
  V3: number;
  V4: number;
  I: number;
  I1: number;
  I2: number;
  I3: number;
  I4: number;
  aT: number;
  cT: number;
  T1: number;
  T2: number;
  T3: number;
  T4: number;
  new_esp_id: number;
  OTC: number;
  Q_low: number;
  Q_high: number;
  I_dschg_max: number;
  wifi: boolean;
}

export const parseDataOnESP32 = (raw: any): BatteryData => ({
  esp_id: raw?.esp_id || 0,
  root_id: raw?.root_id || 0,
  last_updated_time: raw?.last_updated_time || 0,
  live_websocket: raw?.live_websocket || 0,
  t: raw?.t || 0,
  d: raw?.d || 0,
  lat: raw?.lat || 0,
  lon: raw?.lon || 0,
  Q: raw?.Q || 0,
  H: raw?.H || 0,
  V: raw?.V || 0,
  V1: raw?.V1 || 0,
  V2: raw?.V2 || 0,
  V3: raw?.V3 || 0,
  V4: raw?.V4 || 0,
  I: raw?.I || 0,
  I1: raw?.I1 || 0,
  I2: raw?.I2 || 0,
  I3: raw?.I3 || 0,
  I4: raw?.I4 || 0,
  aT: raw?.aT || 0,
  cT: raw?.cT || 0,
  T1: raw?.T1 || 0,
  T2: raw?.T2 || 0,
  T3: raw?.T3 || 0,
  T4: raw?.T4 || 0,
  new_esp_id: 0,
  OTC: raw?.OTC || 0,
  Q_low: raw?.Q_low || 0,
  Q_high: raw?.Q_high || 0,
  I_dschg_max: raw?.I_dschg_max || 0,
  wifi: !!raw?.wifi,
});


// Alert Data Types
export interface AlertData {
  id: string;
  type: 'info' | 'warning' | 'critical';
  message: string;
  batteryId: string;
  batteryName: string;
  timestamp: string;
  isRead: boolean;
}

// User Data Types
export interface UserData {
  id: string;
  name: string;
  email: string;
  role: string;
  avatar: string;
  lastActive: string;
}

export interface User {
  id: number;
  first_name: string;
  last_name: string;
  email: string;
  roles: string[];
}
