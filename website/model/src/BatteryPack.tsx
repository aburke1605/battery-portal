import React, { useMemo } from "react";
import { Canvas } from "@react-three/fiber";
import { Edges, Html, OrbitControls } from "@react-three/drei";
import * as THREE from "three";

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

const BatteryPack3D: React.FC<BatteryPackProps> = ({ cells }) => {
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

export default BatteryPack3D;
