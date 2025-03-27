// Battery Data Types
export interface BatteryData {
  id: string;
  name: string;
  status: 'good' | 'warning' | 'critical' | 'offline';
  IP: string;
  charge: number;
  temperature: number;
  BH: number;
  voltage: number;
  current: number;
  health: number;
  lastMaintenance: string;
  location: string;
  type: string;
  capacity: number;
  cycleCount: number;
  isCharging: boolean;
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