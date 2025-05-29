import React, { useEffect, useMemo, useRef, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Edges, Html, OrbitControls } from "@react-three/drei";
import * as THREE from "three";
import { useAuth } from "../../auth/AuthContext";
import apiConfig from "../../apiConfig";
import { BatteryData } from "../../types";

const cellNSegments = 32;
const cellFrameWidth = 1.0;
const cellFrameHeight = 2.5;

const maxTemp = 80;
const minTemp = 0;
const clamp = (val: number, min: number, max: number) => Math.max(min, Math.min(max, val));

type CellData = {
  label: string;
  position: [number, number, number];
  temperature: number;
  charge: number;
};
export type { CellData };

type BatteryPackProps = {
  cells: CellData[];
}

const getColorFromTemp = (temperature: number): string => {
  const redVal = clamp(Math.round((255 * (temperature - minTemp)) / (maxTemp - minTemp)), 0, 255);
  const blueVal = clamp(Math.round((255 * (maxTemp - temperature)) / (maxTemp - minTemp)), 0, 255);

  return `rgb(${redVal}, 80, ${blueVal})`;
};

const Thermometer: React.FC = () => {
  const height = 2 * cellFrameHeight;

  const gradientTexture = useMemo(() => {
    const canvas = document.createElement("canvas");
    canvas.width = 1;
    canvas.height = 256;
    const ctx = canvas.getContext("2d")!;
    const gradient = ctx.createLinearGradient(0, 0, 0, 256);

    gradient.addColorStop(0.0, getColorFromTemp(maxTemp));
    gradient.addColorStop(0.33, getColorFromTemp(minTemp + (maxTemp-minTemp) * 2 / 3));
    gradient.addColorStop(0.66, getColorFromTemp(minTemp + (maxTemp-minTemp) / 3));
    gradient.addColorStop(1.0, getColorFromTemp(minTemp));

    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, 1, 256);

    const texture = new THREE.CanvasTexture(canvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.needsUpdate = true;
    return texture;
  }, []);

  return (
    <group position={[-5, 0, -5]}>
      {/* colour bar */}
      <mesh position={[0, height / 2, 0]}>
        <boxGeometry args={[0.2, height, 0.2]} />
        <meshStandardMaterial map={gradientTexture} />
      </mesh>

      {/* labels */}
      {[0, 33, 66, 100].map((t) => {
        const y = (t / 100) * height;
        return (
          <Html key={t} position={[0.3, y, 0]} center style={{ fontSize: "10px", color: "white" }}>
            {`${(minTemp + t*(maxTemp-minTemp)/(100-0)).toFixed(1)}°C`}
          </Html>
        );
      })}
    </group>
  );
};

const Cell: React.FC<{ label: string, position: [number, number, number]; temperature: number; charge: number }> = ({ label, position, temperature, charge }) => {
  const fillHeight = Math.max(charge * cellFrameHeight, 0.0);
  const unfilledHeight = cellFrameHeight - fillHeight;
  const color = getColorFromTemp(temperature);
  return (
    <group position={position}>
      {/* unfilled */}
      <mesh position={[0, fillHeight + unfilledHeight / 2, 0]}>
        <cylinderGeometry args={[cellFrameWidth, cellFrameWidth, unfilledHeight, cellNSegments]} />
        <meshStandardMaterial color="white" transparent opacity={0.2} />
        <Edges scale={1.01} threshold={15} />
      </mesh>

      {/* filled */}
      <mesh position={[0, fillHeight / 2, 0]}>
        <cylinderGeometry args={[cellFrameWidth, cellFrameWidth, fillHeight, cellNSegments]} />
        <meshStandardMaterial color={color} />
        <Edges scale={1.01} threshold={15} />
      </mesh>

      {/* label */}
      <Html position={[0, cellFrameHeight + 0.4, 0]} center style={{ pointerEvents: "none", fontSize: "12px", color: "white", fontWeight: "bolder", whiteSpace: "nowrap" }}>
        {label}
      </Html>
    </group>
  );
};

const BatteryPack: React.FC<BatteryPackProps> = ({ cells }) => {
  return (
    <Canvas 
      camera={{ position: [-20, 20, -20], fov: 50 }}
      style={{ background: "#000033" }}
    >
      <ambientLight intensity={5} />
      <pointLight position={[10, 10, 10]} />
      <OrbitControls />

      {cells.map((cell, idx) => (
        <Cell key={idx} label={cell.label} position={cell.position} temperature={cell.temperature} charge={cell.charge} />
      ))}

      <Thermometer />
    </Canvas>
  );
};

interface BatteriesPageProps {
    isFromEsp32?: boolean;
}

export default function DigitalTwin({ isFromEsp32 = false }: BatteriesPageProps) {
    const { getAuthToken } = useAuth();

    let ws_url = apiConfig.WEBSOCKET_BROWSER;
    let queryString = window.location.search;
    if (!isFromEsp32) {
        const hash = window.location.hash;
        queryString = hash.split('?')[1];
    } else {
        ws_url += "?auth_token=" + getAuthToken();
    }
    const urlParams = new URLSearchParams(queryString);
    const esp_id = urlParams.get('esp_id');
    const [batteryItem, setSelectedBattery] = useState<BatteryData | null>(null);
    
    const ws = useRef<WebSocket | null>(null);
    useEffect(() => {
        
        ws.current = new WebSocket(ws_url);

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
                    charge: battery?.Q  || 0,
                    health: battery?.H || 0,
                    voltage: battery?.V/10 || 0,
                    current: battery?.I/10 || 0,
                    temperature: battery?.aT/10 || 0,
                    internal_temperature: battery?.iT/10 || 0,
                    BL: battery?.BL || 0,
                    BH: battery?.BH || 0,
                    CITL: battery?.CITL || 0,
                    CITH: battery?.CITH || 0,
                    CCT: battery?.CCT || 0,
                    DCT: battery?.DCT || 0,
                    isConnected: isFromEsp32 ? (battery?.connected_to_WiFi || false) : (battery?true:false),
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
  
  
  const cells: CellData[] = batteryItem ?
    // websocket data
    ([{ label: "real cell", temperature: batteryItem.temperature, position: [0, 0, 0], charge: batteryItem.charge/100 }])
    :
    // some hardcoded examples
    ([
      { label: "cell 1", temperature: 5, position: [-3, 0, -3], charge: 0.8 },
      { label: "cell 2", temperature: 10, position: [-1, 0, -3], charge: 0.9 },
      { label: "cell 3", temperature: 15, position: [1, 0, -3], charge: 1.0 },
      { label: "cell 4", temperature: 20, position: [3, 0, -3], charge: 0.9 },
      { label: "cell 5", temperature: 25, position: [3, 0, -1], charge: 0.8 },
      { label: "cell 6", temperature: 30, position: [1, 0, -1], charge: 0.7 },
      { label: "cell 7", temperature: 35, position: [-1, 0, -1], charge: 0.6 },
      { label: "cell 8", temperature: 40, position: [-3, 0, -1], charge: 0.5 },
      { label: "cell 9", temperature: 45, position: [-3, 0, 1], charge: 0.4 },
      { label: "cell 10", temperature: 50, position: [-1, 0, 1], charge: 0.3 },
      { label: "cell 11", temperature: 55, position: [1, 0, 1], charge: 0.2 },
      { label: "cell 12", temperature: 60, position: [3, 0, 1], charge: 0.1 },
      { label: "cell 13", temperature: 65, position: [3, 0, 3], charge: 0.0 },
      { label: "cell 14", temperature: 70, position: [1, 0, 3], charge: 0.1 },
      { label: "cell 15", temperature: 75, position: [-1, 0, 3], charge: 0.2 },
      { label: "cell 16", temperature: 80, position: [-3, 0, 3], charge: 0.3 },
  ]);

  return (
    <div style={{ height: "100vh" }}>
      <BatteryPack cells={cells} />
    </div>
  );
}
