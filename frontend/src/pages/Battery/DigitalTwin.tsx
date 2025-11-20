import React, {
	useCallback,
	useEffect,
	useMemo,
	useRef,
	useState,
} from "react";
import {
	BrainCircuit,
	ArrowBigRightDash,
	X,
	SquareCheckBig,
} from "lucide-react";
import { Canvas } from "@react-three/fiber";
import { Edges, Html, OrbitControls } from "@react-three/drei";
import * as THREE from "three";
import { useAuth } from "../../auth/AuthContext";
import apiConfig from "../../apiConfig";
import { BatteryData } from "../../types";
import { useLoader } from "@react-three/fiber";
import { SVGLoader } from "three-stdlib";
import {
	createMessage,
	fetchBatteryData,
	useWebSocket,
} from "../../hooks/useWebSocket";
import { generate_random_string } from "../../utils/helpers";
import { useNavigate } from "react-router";
import axios from "axios";

const cellFrameWidthX = 2.5;
const cellFrameWidthY = 1.5;
const cellFrameHeight = 2.0;

const maxTemp = 30;
const minTemp = 20;
const clamp = (val: number, min: number, max: number) =>
	Math.max(min, Math.min(max, val));

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
	esp_id: number;
	connected: boolean;
	battery: BatteryData;
	sendBatteryUpdate: (updatedValues: Partial<BatteryData>) => void;
};

const getColorFromTemp = (temperature: number): string => {
	const redVal = clamp(
		Math.round((255 * (temperature - minTemp)) / (maxTemp - minTemp)),
		0,
		255,
	);
	const blueVal = clamp(
		Math.round((255 * (maxTemp - temperature)) / (maxTemp - minTemp)),
		0,
		255,
	);

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
		gradient.addColorStop(
			0.33,
			getColorFromTemp(minTemp + ((maxTemp - minTemp) * 2) / 3),
		);
		gradient.addColorStop(
			0.66,
			getColorFromTemp(minTemp + (maxTemp - minTemp) / 3),
		);
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
					<Html
						key={t}
						position={[0.3, y, 0]}
						center
						style={{ fontSize: "10px", color: "white" }}
					>
						{`${(minTemp + (t * (maxTemp - minTemp)) / (100 - 0)).toFixed(1)}°C`}
					</Html>
				);
			})}
		</group>
	);
};

const ChargingBolt = () => {
	const svg = useLoader(SVGLoader, "/bolt.svg");

	// Extract shapes from SVG
	const shapes = useMemo(() => {
		return svg.paths.flatMap((path) => path.toShapes(true));
	}, [svg]);

	return (
		<group
			position={[0.1, cellFrameHeight + 0.9, 0.1]}
			scale={[0.001, 0.001, 0.001]}
		>
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
					<meshStandardMaterial
						color="yellow"
						metalness={0.5}
						roughness={0.2}
					/>
				</mesh>
			))}
		</group>
	);
};

const Cell: React.FC<CellData> = ({
	label,
	position,
	temperature,
	charge,
	isCharging,
}) => {
	const fillHeight = Math.max(charge * cellFrameHeight, 0.0);
	const unfilledHeight = cellFrameHeight - fillHeight;
	const color = getColorFromTemp(temperature);
	return (
		<group position={position}>
			{/* unfilled */}
			<mesh position={[0, fillHeight + unfilledHeight / 2, 0]}>
				<boxGeometry
					args={[cellFrameWidthX, unfilledHeight, cellFrameWidthY]}
				/>
				<meshStandardMaterial color="white" transparent opacity={0.2} />
				<Edges scale={1.01} threshold={15} />
			</mesh>

			{/* filled */}
			<mesh position={[0, fillHeight / 2, 0]}>
				<boxGeometry args={[cellFrameWidthX, fillHeight, cellFrameWidthY]} />
				<meshStandardMaterial color={color} />
				<Edges scale={1.01} threshold={15} />
			</mesh>

			{/* label */}
			<Html
				position={[0, cellFrameHeight + 0.4, 0]}
				center
				style={{
					pointerEvents: "none",
					fontSize: "12px",
					color: "white",
					fontWeight: "bolder",
					whiteSpace: "nowrap",
				}}
			>
				{label}
			</Html>

			{isCharging ? <ChargingBolt /> : null}
		</group>
	);
};

const BatteryPack: React.FC<BatteryPackProps> = ({
	cells,
	esp_id,
	connected,
	battery,
	sendBatteryUpdate, // receive function from BatteryDetail
}) => {
	interface Recommendation {
		type: string;
		message: string;
		min?: number;
		max?: number;
		implementing: boolean;
		delay: boolean;
		implemented: boolean;
		success: boolean;
	}
	const [recommendationCards, setRecommendationCards] = useState<
		Recommendation[] | null
	>(null);
	async function getRecommendations() {
		const response = await axios.get(
			`${apiConfig.DB_RECOMMENDATION_API}?esp_id=${battery.esp_id}`,
		);

		const recommendations: Recommendation[] = (
			response.data.recommendations || []
		).map((rec: any) => ({
			...rec,
			implementing: false,
			delay: false,
			implemented: false,
			success: true, // assumption
		}));
		setRecommendationCards(recommendations);
	}

	async function implementRecommendation(recommendation: Recommendation) {
		if (recommendation.implementing) return; // already clicked the button recently
		if (!recommendationCards) return;

		switch (recommendation.type) {
			case "current-dischg-limit": {
				await processRecommendation(
					recommendation,
					{ I_dschg_max: recommendation.max },
					() => battery.I_dschg_max === recommendation.max,
				);
				break;
			}

			case "soc-window":
				await processRecommendation(
					recommendation,
					{ Q_low: recommendation.min, Q_high: recommendation.max },
					() =>
						battery.Q_low === recommendation.min &&
						battery.Q_high === recommendation.max,
				);
				break;

			case "operating-voltage":
				await processRecommendation(
					recommendation,
					{ V_max: recommendation.max },
					() => battery.V_max === recommendation.max,
				);
				break;

			default:
				console.log("default");
		}
	}

	function useBatteryWaiter(battery: BatteryData) {
		// hook for waiting until next battery object update
		const resolvers = useRef<(() => void)[]>([]);

		useEffect(() => {
			// resolve all waiting promises when battery object changes
			resolvers.current.forEach((r) => r());
			resolvers.current = [];
		}, [battery]);

		function awaitNextUpdate(): Promise<void> {
			return new Promise((resolve) => {
				resolvers.current.push(resolve);
			});
		}

		return awaitNextUpdate;
	}

	const sleep = (ms: number) =>
		new Promise((resolve) => setTimeout(resolve, ms));
	const awaitNextBatteryUpdate = useBatteryWaiter(battery);

	async function processRecommendation(
		recommendation: Recommendation,
		values: Partial<BatteryData>,
		check: () => boolean,
	) {
		// initialise boolean flags
		setRecommendationCards(
			(prev) =>
				prev?.map((rec) =>
					rec.type === recommendation.type
						? { ...rec, implementing: true, delay: false, success: true }
						: rec,
				) ?? null,
		);

		// send the recommended changes back through WebSocket to device
		sendBatteryUpdate(values);

		// confirm completion by waiting for new battery data...
		await sleep(5000); // ...after a brief delay so immediate updates don't interfere
		setRecommendationCards(
			(prev) =>
				prev?.map((rec) =>
					rec.type === recommendation.type ? { ...rec, delay: true } : rec,
				) ?? null,
		);
		await awaitNextBatteryUpdate();

		// update boolean flags on result
		if (check()) {
			setRecommendationCards(
				(prev) =>
					prev?.map((rec) =>
						rec.type === recommendation.type
							? { ...rec, implemented: true }
							: rec,
					) ?? null,
			);
		} else {
			setRecommendationCards(
				(prev) =>
					prev?.map((rec) =>
						rec.type === recommendation.type
							? { ...rec, implementing: false, delay: false, success: false }
							: rec,
					) ?? null,
			);
		}
	}

	function removeRecommendation(recommendation: Recommendation) {
		setRecommendationCards((prev) => {
			const updated = prev ? prev.filter((r) => r !== recommendation) : prev;
			return updated && updated.length > 0 ? updated : null; // allows button to be clicked again
		});
	}

	const shift = 450;

	const [isDragging, setIsDragging] = useState(false);

	return (
		<div
			className="relative w-full h-full"
			style={{ paddingRight: `${shift}px` }} // space reserved for overlay
		>
			<Canvas
				className={isDragging ? "cursor-grabbing" : "cursor-grab"}
				onPointerDown={() => setIsDragging(true)}
				onPointerUp={() => setIsDragging(false)}
				camera={{ position: [-20, 20, -20], fov: 50 }}
				style={{ background: "#000033" }}
			>
				<ambientLight intensity={5} />
				<pointLight position={[10, 10, 10]} />
				<OrbitControls />

				{cells.map((cell, idx) => (
					<Cell
						key={idx}
						label={cell.label}
						position={cell.position}
						temperature={cell.temperature}
						charge={cell.charge}
						isCharging={cell.isCharging}
					/>
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
					Welcome to digital twin view!
					<br />
					{!connected ? <>Loading battery data...</> : <>BMS ID: {esp_id}</>}
				</Html>

				<Thermometer />
			</Canvas>
			<div
				className="absolute top-0 right-0 h-full bg-white text-black p-4 shadow-xl"
				style={{ width: `${shift}px` }}
			>
				<div>
					<button
						onClick={() => (recommendationCards ? null : getRecommendations())}
						className="w-full flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 mb-4"
					>
						<BrainCircuit size={16} className="mr-2" />
						Get Recommendations
					</button>
					{recommendationCards?.map((recommendation) => (
						<div className="flex items-center justify-between mb-2">
							<span>{recommendation.message}</span>
							<div className="flex gap-2">
								{recommendation.implemented === false ? (
									<>
										<button
											onClick={() => implementRecommendation(recommendation)}
											className="w-auto flex items-center justify-center px-4 py-2 border border-green-300 shadow-sm text-sm font-medium rounded-md text-green-700 bg-green-50 hover:bg-green-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500"
										>
											{recommendation.implementing === false ? (
												<>
													{recommendation.success === true ? (
														<>Implement</>
													) : (
														<>Failed! Try again...</>
													)}
													<ArrowBigRightDash size={16} className="ml-2" />
												</>
											) : (
												<>
													{recommendation.delay === false ? (
														<>Implementing...</>
													) : (
														<>Waiting for confirmation...</>
													)}
												</>
											)}
										</button>
										<button
											onClick={() => removeRecommendation(recommendation)}
											className="w-auto flex items-center justify-center px-4 py-2 border border-red-300 shadow-sm text-sm font-medium rounded-md text-red-700 bg-red-50 hover:bg-red-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500"
										>
											<X size={16} />
										</button>
									</>
								) : (
									<>
										<button
											onClick={() => removeRecommendation(recommendation)}
											className="w-auto flex items-center justify-center px-4 py-2 border border-green-300 shadow-sm text-sm font-medium rounded-md text-green-700 bg-green-50 hover:bg-green-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500"
										>
											Implemented
											<SquareCheckBig size={16} className="ml-2" />
										</button>
									</>
								)}
							</div>
						</div>
					))}
				</div>
			</div>
		</div>
	);
};

interface BatteriesPageProps {
	isFromESP32?: boolean;
}

export default function DigitalTwin({
	isFromESP32 = false,
}: BatteriesPageProps) {
	const { getAuthToken } = useAuth();

	let ws_url = apiConfig.WEBSOCKET_BROWSER;
	let queryString = window.location.search;
	if (!isFromESP32) {
		const hash = window.location.hash;
		queryString = hash.split("?")[1];
	}
	const urlParams = new URLSearchParams(queryString);
	const esp_id = Number(urlParams.get("esp_id"));

	const ws_session_browser_id = useRef(generate_random_string(32));
	ws_url = isFromESP32
		? (ws_url += "?auth_token=" + getAuthToken())
		: (ws_url +=
				"?browser_id=" + ws_session_browser_id.current + "&esp_id=" + esp_id);

	const [battery, setBatteryData] = useState<BatteryData | null>(null);

	const navigate = useNavigate();
	const viewDetails = (battery: BatteryData) => {
		navigate(`/battery-detail?esp_id=${battery.esp_id}`);
	};

	// get data from DB
	useEffect(() => {
		const loadBattery = async () => {
			const esp = await fetchBatteryData(esp_id);
			if (esp !== null) setBatteryData(esp);
		};

		loadBattery();
	}, []);

	// update data after WebSocket trigger
	const handleMessage = useCallback(
		async (data: any) => {
			if (
				data.esp_id === esp_id &&
				data.browser_id === ws_session_browser_id.current
			) {
				if (data.type === "status_update") {
					const esp = await fetchBatteryData(esp_id);
					if (esp !== null) setBatteryData(esp);
				}
			}
		},
		[esp_id],
	);
	const { sendMessage } = useWebSocket({
		url: ws_url,
		onMessage: handleMessage,
	});

	const sleep = (ms: number) =>
		new Promise((resolve) => setTimeout(resolve, ms));

	const sendBatteryUpdate = async (updates: Partial<BatteryData>) => {
		sendMessage(createMessage("change-settings", updates, esp_id));
		if (updates.new_esp_id && updates.new_esp_id !== esp_id) {
			await sleep(5000);
			!isFromESP32
				? window.location.replace(
						`/#/digital-twin?esp_id=${updates.new_esp_id}`,
					)
				: (window.location.href = `/esp32?esp_id=${updates.new_esp_id}#/`);
		}
	};

	const cells: CellData[] = battery
		? // websocket data
			[
				{
					label: "cell 1",
					temperature: battery.T1,
					position: [-1.5 * cellFrameWidthX, 0, -1.5 * cellFrameWidthY],
					charge: battery.Q / 100,
					isCharging: battery.I1 > 0 ? true : false,
				},
				{
					label: "cell 2",
					temperature: battery.T2,
					position: [-1.5 * cellFrameWidthX, 0, -0.5 * cellFrameWidthY],
					charge: battery.Q / 100,
					isCharging: battery.I2 > 0 ? true : false,
				},
				{
					label: "cell 3",
					temperature: battery.T3,
					position: [-1.5 * cellFrameWidthX, 0, 0.5 * cellFrameWidthY],
					charge: battery.Q / 100,
					isCharging: battery.I3 > 0 ? true : false,
				},
				{
					label: "cell 4",
					temperature: battery.T4,
					position: [-1.5 * cellFrameWidthX, 0, 1.5 * cellFrameWidthY],
					charge: battery.Q / 100,
					isCharging: battery.I4 > 0 ? true : false,
				},
			]
		: // some hardcoded examples
			[
				{
					label: "cell 1",
					temperature: 5,
					position: [-1.5 * cellFrameWidthX, 0, -1.5 * cellFrameWidthY],
					charge: 0.8,
					isCharging: false,
				},
				{
					label: "cell 2",
					temperature: 10,
					position: [-0.5 * cellFrameWidthX, 0, -1.5 * cellFrameWidthY],
					charge: 0.9,
					isCharging: false,
				},
				{
					label: "cell 3",
					temperature: 15,
					position: [0.5 * cellFrameWidthX, 0, -1.5 * cellFrameWidthY],
					charge: 1.0,
					isCharging: false,
				},
				{
					label: "cell 4",
					temperature: 20,
					position: [1.5 * cellFrameWidthX, 0, -1.5 * cellFrameWidthY],
					charge: 0.9,
					isCharging: false,
				},
				{
					label: "cell 5",
					temperature: 25,
					position: [1.5 * cellFrameWidthX, 0, -0.5 * cellFrameWidthY],
					charge: 0.8,
					isCharging: false,
				},
				{
					label: "cell 6",
					temperature: 30,
					position: [0.5 * cellFrameWidthX, 0, -0.5 * cellFrameWidthY],
					charge: 0.7,
					isCharging: false,
				},
				{
					label: "cell 7",
					temperature: 35,
					position: [-0.5 * cellFrameWidthX, 0, -0.5 * cellFrameWidthY],
					charge: 0.6,
					isCharging: false,
				},
				{
					label: "cell 8",
					temperature: 40,
					position: [-1.5 * cellFrameWidthX, 0, -0.5 * cellFrameWidthY],
					charge: 0.5,
					isCharging: false,
				},
				{
					label: "cell 9",
					temperature: 45,
					position: [-1.5 * cellFrameWidthX, 0, 0.5 * cellFrameWidthY],
					charge: 0.4,
					isCharging: false,
				},
				{
					label: "cell 10",
					temperature: 50,
					position: [-0.5 * cellFrameWidthX, 0, 0.5 * cellFrameWidthY],
					charge: 0.3,
					isCharging: false,
				},
				{
					label: "cell 11",
					temperature: 55,
					position: [0.5 * cellFrameWidthX, 0, 0.5 * cellFrameWidthY],
					charge: 0.2,
					isCharging: false,
				},
				{
					label: "cell 12",
					temperature: 60,
					position: [1.5 * cellFrameWidthX, 0, 0.5 * cellFrameWidthY],
					charge: 0.1,
					isCharging: false,
				},
				{
					label: "cell 13",
					temperature: 65,
					position: [1.5 * cellFrameWidthX, 0, 1.5 * cellFrameWidthY],
					charge: 0.0,
					isCharging: false,
				},
				{
					label: "cell 14",
					temperature: 70,
					position: [0.5 * cellFrameWidthX, 0, 1.5 * cellFrameWidthY],
					charge: 0.1,
					isCharging: false,
				},
				{
					label: "cell 15",
					temperature: 75,
					position: [-0.5 * cellFrameWidthX, 0, 1.5 * cellFrameWidthY],
					charge: 0.2,
					isCharging: false,
				},
				{
					label: "cell 16",
					temperature: 80,
					position: [-1.5 * cellFrameWidthX, 0, 1.5 * cellFrameWidthY],
					charge: 0.3,
					isCharging: false,
				},
			];

	return (
		<div style={{ height: "80vh" }} className="flex flex-col items-center">
			{battery ? (
				<BatteryPack
					cells={cells}
					esp_id={battery ? battery.esp_id : 0}
					connected={battery ? true : false}
					battery={battery}
					sendBatteryUpdate={sendBatteryUpdate} // pass function to BatteryPack
				/>
			) : (
				<p> Connecting to battery... </p> // fallback UI
			)}
			<button
				className="w-full max-w-xs flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 mt-4"
				onClick={() => battery && viewDetails(battery)}
			>
				Back
			</button>
		</div>
	);
}
