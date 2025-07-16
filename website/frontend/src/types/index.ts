// Battery Data Types
export interface BatteryData {
  esp_id: string;
  new_esp_id: string;
  status: 'good' | 'warning' | 'critical' | 'offline';
  charge: number;
  ambient_temperature: number;
  cell_temperature: number;
  OCT_threshold: number;
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
  cell1_current: number;
  cell2_current: number;
  cell3_current: number;
  cell4_current: number;
  cell1_temperature: number;
  cell2_temperature: number;
  cell3_temperature: number;
  cell4_temperature: number;
  cell1_voltage: number;
  cell2_voltage: number;
  cell3_voltage: number;
  cell4_voltage: number;
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
  ambient_temperature: raw?.aT / 10 || 0,
  cell_temperature: raw?.cT / 10 || 0,
  OCT_threshold: raw?.OCT_threshold || 0,
  isConnected: isFromEsp32 ? !!raw?.connected_to_WiFi : !!raw,
  status: "good",
  timestamp: Date.now(),
  cell1_current: raw?.I1 / 100 || 0,
  cell2_current: raw?.I2 / 100 || 0,
  cell3_current: raw?.I3 / 100 || 0,
  cell4_current: raw?.I4 / 100 || 0,
  cell1_temperature: raw?.T1 / 100 || 0,
  cell2_temperature: raw?.T2 / 100 || 0,
  cell3_temperature: raw?.T3 / 100 || 0,
  cell4_temperature: raw?.T4 / 100 || 0,
  cell1_voltage: raw?.V1 / 100 || 0,
  cell2_voltage: raw?.V2 / 100 || 0,
  cell3_voltage: raw?.V3 / 100 || 0,
  cell4_voltage: raw?.V4 / 100 || 0,
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