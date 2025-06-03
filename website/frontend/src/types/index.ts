// Battery Data Types
export interface BatteryData {
  esp_id: string;
  new_esp_id: string;
  status: 'good' | 'warning' | 'critical' | 'offline';
  charge: number;
  temperature: number;
  internal_temperature: number;
  BL: number;
  BH: number;
  CITL: number;
  CITH: number;
  CCT: number;
  DCT: number;
  voltage: number;
  current: number;
  health: number;
  // lastMaintenance: string;
  // location: string;
  // type: string;
  // capacity: number;
  // cycleCount: number;
  // isCharging: boolean;
  timestamp: number;
  isConnected: boolean;
}

export const parseBatteryData = (
  esp_id: string,
  raw: any,
  isFromEsp32 = false
): BatteryData => ({
  esp_id,
  new_esp_id: "",
  charge: raw?.Q || 0,
  health: raw?.H || 0,
  voltage: raw?.V / 10 || 0,
  current: raw?.I / 10 || 0,
  temperature: raw?.aT / 10 || 0,
  internal_temperature: raw?.iT / 10 || 0,
  BL: raw?.BL || 0,
  BH: raw?.BH || 0,
  CITL: raw?.CITL || 0,
  CITH: raw?.CITH || 0,
  CCT: raw?.CCT || 0,
  DCT: raw?.DCT || 0,
  isConnected: isFromEsp32 ? !!raw?.connected_to_WiFi : !!raw,
  status: "good",
  timestamp: Date.now(),
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