import React, { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Edges, Html, OrbitControls } from "@react-three/drei";
import * as THREE from "three";
import { useAuth } from "../../auth/AuthContext";
import apiConfig from "../../apiConfig";
import { BatteryData } from "../../types";
import { useLoader } from '@react-three/fiber'
import { SVGLoader } from 'three-stdlib'
import { fetchBatteryData, useWebSocket } from "../../hooks/useWebSocket";
import { generate_random_string } from "../../utils/helpers";

const cellNSegments = 32;
const cellFrameWidth = 1.0;
const cellFrameHeight = 2.5;

const maxTemp = 30;
const minTemp = 20;
const clamp = (val: number, min: number, max: number) => Math.max(min, Math.min(max, val));

type CellData = {
  label: string;
  position: [number, number, number];
  temperature: number;
  charge: number;
  isCharging: boolean;
};
export type { CellData };

type BatteryPackProps = {
  cells: CellData[];
  esp_id: string;
  connected: boolean;
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

const ChargingBolt = () => {
  const svg = useLoader(SVGLoader, '/bolt.svg')

  // Extract shapes from SVG
  const shapes = useMemo(() => {
    return svg.paths.flatMap(path => path.toShapes(true))
  }, [svg])

  return (
    <group position={[0.1, cellFrameHeight + 0.9, 0.1]} scale={[0.001, 0.001, 0.001]}>
      {shapes.map((shape, index) => (
        <mesh key={index} castShadow receiveShadow>
          <extrudeGeometry
            args={[
              shape,
              {
                depth: 20,
                bevelEnabled: true,
                bevelSegments: 5,
                steps: 1,
                bevelSize: 1,
                bevelThickness: 5,
              },
            ]}
          />
          <meshStandardMaterial color="yellow" metalness={0.5} roughness={0.2} />
        </mesh>
      ))}
    </group>
  )
}

const Cell: React.FC<CellData> = ({ label, position, temperature, charge, isCharging }) => {
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

      {isCharging ? (
        <ChargingBolt />
      ) : null}
    </group>
  );
};

const BatteryPack: React.FC<BatteryPackProps> = ({ cells, esp_id, connected }) => {
  return (
    <Canvas 
      camera={{ position: [-20, 20, -20], fov: 50 }}
      style={{ background: "#000033" }}
    >
      <ambientLight intensity={5} />
      <pointLight position={[10, 10, 10]} />
      <OrbitControls />

      {cells.map((cell, idx) => (
        <Cell key={idx} label={cell.label} position={cell.position} temperature={cell.temperature} charge={cell.charge} isCharging={cell.isCharging} />
      ))}

      <Html
        position={[0, 3 * cellFrameHeight, 0]}
        center
        style={{
          pointerEvents: "none",
          fontSize: "20px",
          color: "white",
          fontWeight: "bolder",
          textAlign: "center",
          whiteSpace: "nowrap",
        }}
      >
        {!connected ? (
          <>
            Welcome to digital twin view!
            <br />
            Loading battery data...
          </>
        ) :
          <>
          {esp_id}
          </>
        }
      </Html>

      <Thermometer />
    </Canvas>
  );
};

interface BatteriesPageProps {
    isFromESP32?: boolean;
}

export default function DigitalTwin({ isFromESP32 = false }: BatteriesPageProps) {
    const { getAuthToken } = useAuth();

    let ws_url = apiConfig.WEBSOCKET_BROWSER;
    let queryString = window.location.search;
    if (!isFromESP32) {
        const hash = window.location.hash;
        queryString = hash.split('?')[1];
    }
    const urlParams = new URLSearchParams(queryString);
    let esp_id = urlParams.get('esp_id');
    if (esp_id == null) esp_id = "empty";

    const ws_session_browser_id = useRef(generate_random_string(32));
    ws_url = isFromESP32 ? ws_url += "?auth_token=" + getAuthToken() : ws_url += "?browser_id=" + ws_session_browser_id.current + "&esp_id=" + esp_id;

    const [battery, setBatteryData] = useState<BatteryData | null>(null);

    // get data from DB
    useEffect(() => {
        const loadBattery = async () => {
            const esp = await fetchBatteryData(esp_id);
            if (esp !== null) setBatteryData(esp);
        };

        loadBattery();
    }, []);

    // update data after WebSocket trigger
    const handleMessage = useCallback(async (data: any) => {
        if (data.esp_id === esp_id && data.browser_id === ws_session_browser_id.current) {
            if (data.type === "status_update") {
                const esp = await fetchBatteryData(esp_id);
                if (esp !== null) setBatteryData(esp);
            }
        }
    }, [esp_id]);
    useWebSocket({
        url: ws_url,
        onMessage: handleMessage,
    });
  
  
  const cells: CellData[] = battery ?
    // websocket data
    ([
      { label: "cell 1", temperature: battery.T1, position: [-3, 0, -3], charge: battery.Q/100, isCharging: battery.I1>0?true:false },
      { label: "cell 2", temperature: battery.T2, position: [-3, 0, -1], charge: battery.Q/100, isCharging: battery.I2>0?true:false },
      { label: "cell 3", temperature: battery.T3, position: [-3, 0, 1], charge: battery.Q/100, isCharging: battery.I3>0?true:false },
      { label: "cell 4", temperature: battery.T4, position: [-3, 0, 3], charge: battery.Q/100, isCharging: battery.I4>0?true:false },
    ])
    :
    // some hardcoded examples
    ([
      { label: "cell 1", temperature: 5, position: [-3, 0, -3], charge: 0.8, isCharging: false },
      { label: "cell 2", temperature: 10, position: [-1, 0, -3], charge: 0.9, isCharging: false },
      { label: "cell 3", temperature: 15, position: [1, 0, -3], charge: 1.0, isCharging: false },
      { label: "cell 4", temperature: 20, position: [3, 0, -3], charge: 0.9, isCharging: false },
      { label: "cell 5", temperature: 25, position: [3, 0, -1], charge: 0.8, isCharging: false },
      { label: "cell 6", temperature: 30, position: [1, 0, -1], charge: 0.7, isCharging: false },
      { label: "cell 7", temperature: 35, position: [-1, 0, -1], charge: 0.6, isCharging: false },
      { label: "cell 8", temperature: 40, position: [-3, 0, -1], charge: 0.5, isCharging: false },
      { label: "cell 9", temperature: 45, position: [-3, 0, 1], charge: 0.4, isCharging: false },
      { label: "cell 10", temperature: 50, position: [-1, 0, 1], charge: 0.3, isCharging: false },
      { label: "cell 11", temperature: 55, position: [1, 0, 1], charge: 0.2, isCharging: false },
      { label: "cell 12", temperature: 60, position: [3, 0, 1], charge: 0.1, isCharging: false },
      { label: "cell 13", temperature: 65, position: [3, 0, 3], charge: 0.0, isCharging: false },
      { label: "cell 14", temperature: 70, position: [1, 0, 3], charge: 0.1, isCharging: false },
      { label: "cell 15", temperature: 75, position: [-1, 0, 3], charge: 0.2, isCharging: false },
      { label: "cell 16", temperature: 80, position: [-3, 0, 3], charge: 0.3, isCharging: false },
  ]);

  return (
    <div style={{ height: "100vh" }}>
      <BatteryPack cells={cells} esp_id={battery?battery.esp_id:"simulation"} connected={battery?true:false}/>
    </div>
  );
}
