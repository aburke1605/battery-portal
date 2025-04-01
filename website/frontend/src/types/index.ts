// Battery Data Types
export interface BatteryData {
  esp_id: string;
  status: 'good' | 'warning' | 'critical' | 'offline';
  IP: string;
  charge: number;
  temperature: number;
  BL: number;
  BH: number;
  CITL: number;
  CITH: number;
  CCT: number;
  DCT: number;
  voltage: number;
  current: number;
  health: number;
  lastMaintenance: string;
  location: string;
  type: string;
  capacity: number;
  cycleCount: number;
  isCharging: boolean;
  timestamp: number;
  isConnected: boolean;
}


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