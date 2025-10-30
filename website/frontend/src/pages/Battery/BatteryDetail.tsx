import React, { useEffect, useRef, useState } from "react";
import { BatteryData } from "../../types";
import {
	BatteryLow,
	BatteryMedium,
	BatteryFull,
	Zap,
	ThermometerSun,
	Info,
	RefreshCw,
	LockKeyholeOpen,
	Wifi,
	BrainCircuit,
	BarChart3,
	Sliders,
	Battery,
	Layers2,
	ArrowBigRightDash,
	X,
	SquareCheckBig,
	Clock,
} from "lucide-react";
import { getStatusColor } from "../../utils/helpers";
import { useNavigate } from "react-router-dom";
import axios from "axios";
import apiConfig from "../../apiConfig";

interface BatteryDetailProps {
	battery: BatteryData;
	// onToggleCharging: (batteryId: string) => void;
	voltageThreshold: number;
	sendBatteryUpdate: (updatedValues: Partial<BatteryData>) => void;
	sendWiFiConnect: (
		ssid: string,
		password: string,
		auto_connect: boolean,
	) => void;
	sendUnseal: () => void;
	sendReset: () => void;
	isFromESP32: boolean;
	updateRequest: () => void;
}

const BatteryDetail: React.FC<BatteryDetailProps> = ({
	battery,
	voltageThreshold,
	sendBatteryUpdate, // receive function from BatteryPage
	sendWiFiConnect,
	sendUnseal,
	sendReset,
	isFromESP32,
}) => {
	const [activeTab, setActiveTab] = useState("overview");

	const [isEditing, setIsEditing] = useState(false);
	const [hasChanges, setHasChanges] = useState(false);
	const [hasWiFiChanges, setHasWiFiChanges] = useState(false);

	const [ssid, setSSID] = useState("");
	const [password, setPassword] = useState("");
	const [autoConnect, setAutoConnect] = useState(false);

	// range
	const OTC_threshold_min = 450;
	const OTC_threshold_max = 650;
	// initialise
	const [values, setValues] = useState<Partial<BatteryData>>({
		esp_id: battery.esp_id,
		OTC: battery.OTC,
	});
	// websocket update
	useEffect(() => {
		if (!isEditing) {
			setValues({
				esp_id: battery.esp_id,
				OTC: battery.OTC,
			});
			setHasChanges(false);
		}
	}, [battery.esp_id, battery.OTC, isEditing]);
	// slider update
	const handleSliderChange =
		(key: string) => (e: React.ChangeEvent<HTMLInputElement>) => {
			const { value, type } = e.target;
			const newValue =
				type === "number" || type === "range" ? Number(value) : value; // Convert numbers only for sliders
			setValues((prevValues) => {
				const updatedValues = { ...prevValues, [key]: newValue };
				// check if any slider has moved
				const hasAnyChange = (
					Object.keys(values) as Array<keyof BatteryData>
				).some((k) => values[k] !== battery[k as keyof BatteryData]);
				setHasChanges(hasAnyChange);
				setIsEditing(true);
				return updatedValues;
			});
		};
	// send websocket-update message
	const handleSubmit = () => {
		sendBatteryUpdate(values);
		setIsEditing(false);
		setHasChanges(false);
	};
	// reset sliders
	const handleReset = () => {
		setValues({
			OTC: battery.OTC,
		});
		setIsEditing(false);
		setHasChanges(false);
	};
	// router entry
	const handleTextChange =
		(setter: React.Dispatch<React.SetStateAction<string>>) =>
		(e: React.ChangeEvent<HTMLInputElement>) => {
			setter(e.target.value);
			setHasWiFiChanges(true);
		};
	const handleCheckboxChange =
		(setter: React.Dispatch<React.SetStateAction<boolean>>) =>
		(e: React.ChangeEvent<HTMLInputElement>) => {
			setter(e.target.checked);
			setHasWiFiChanges(true);
		};
	// send websocket-connect message
	const handleConnect = () => {
		sendWiFiConnect(ssid, password, autoConnect);
		setSSID("");
		setPassword("");
		setHasWiFiChanges(false);
	};

	const renderTabContent = () => {
		switch (activeTab) {
			case "settings":
				return (
					<div className="space-y-6">
						<div className="bg-white border border-gray-200 rounded-lg shadow-sm">
							<div className="px-4 py-5 sm:px-6 border-b border-gray-200">
								<h3 className="text-lg font-medium text-gray-900 flex items-center">
									<Sliders size={20} className="mr-2" /> Battery Settings
								</h3>
							</div>
							<div className="p-6">
								<div className="space-y-6">
									<div>
										<h4 className="text-sm font-medium text-gray-900 mb-4">
											Charging Parameters
										</h4>
										<div className="space-y-4">
											<div>
												<label className="block text-sm font-medium text-gray-700">
													Device ID:
												</label>
												<input
													type="number"
													className="border p-1 w-full"
													defaultValue={battery.esp_id}
													onChange={handleSliderChange("new_esp_id")}
												/>

												<label className="block text-sm font-medium text-gray-700">
													OCT threshold: {values.OTC} [0.1 °C]
												</label>
												<input
													type="range"
													className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer mt-2"
													min={OTC_threshold_min}
													max={OTC_threshold_max}
													value={values.OTC}
													onChange={handleSliderChange("OTC")}
												/>
												<div className="flex justify-between text-xs text-gray-500 mt-1">
													<span>{OTC_threshold_min} [0.1 °C]</span>
													<span>{OTC_threshold_max} [0.1 °C]</span>
												</div>

												{hasChanges && (
													<div className="flex gap-2 mt-2">
														{isFromESP32 || battery.live_websocket ? (
															<>
																<button
																	onClick={handleSubmit}
																	className="p-2 bg-blue-500 text-white rounded"
																>
																	Submit Updates
																</button>
																<button
																	onClick={handleReset}
																	className="p-2 bg-gray-500 text-white rounded"
																>
																	Reset
																</button>
															</>
														) : (
															<button className="p-2 bg-gray-500 text-white rounded">
																OFFLINE
															</button>
														)}
													</div>
												)}
											</div>
										</div>
									</div>
								</div>
							</div>
						</div>
					</div>
				);

			case "wifi":
				return (
					<div className="space-y-6">
						<div className="bg-white border border-gray-200 rounded-lg shadow-sm">
							<div className="px-4 py-5 sm:px-6 border-b border-gray-200">
								<h3 className="text-lg font-medium text-gray-900 flex items-center">
									<Sliders size={20} className="mr-2" /> Wi-Fi Settings
								</h3>
							</div>
							<div className="p-6">
								<div className="space-y-6">
									<div>
										<h4 className="text-sm font-medium text-gray-900 mb-4">
											Router Information
										</h4>
										<div className="space-y-4">
											<div>
												<label className="block text-sm font-medium text-gray-700">
													SSID:
												</label>
												<input
													type="text"
													className="border p-1 w-full"
													value={ssid}
													name="ssid"
													id="ssid"
													required
													onChange={handleTextChange(setSSID)}
												/>

												<label className="block text-sm font-medium text-gray-700">
													Password:
												</label>
												<input
													type="password"
													className="border p-1 w-full"
													value={password}
													name="password"
													id="password"
													required
													onChange={handleTextChange(setPassword)}
												/>
											</div>

											<div>
												<label className="flex items-center space-x-2 text-sm font-medium text-gray-700 mt-2">
													<input
														type="checkbox"
														checked={autoConnect}
														onChange={handleCheckboxChange(setAutoConnect)}
														className="h-4 w-4 border-gray-300 rounded"
													/>
													<span>Auto-connect</span>
												</label>
											</div>

											{hasWiFiChanges && (
												<div className="flex gap-2 mt-2">
													{isFromESP32 || battery.live_websocket ? (
														<button
															onClick={handleConnect}
															className="p-2 bg-blue-500 text-white rounded"
														>
															Connect
														</button>
													) : (
														<button className="p-2 bg-gray-500 text-white rounded">
															OFFLINE
														</button>
													)}
												</div>
											)}
										</div>
									</div>
								</div>
							</div>
						</div>
					</div>
				);

			default: // Overview tab
				return (
					<div className="space-y-6">
						<div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
							{/* Left Column - Detailed Stats */}
							<div className="lg:col-span-2 space-y-6">
								<div className="bg-white border border-gray-200 rounded-lg shadow-sm">
									<div className="px-4 py-5 sm:px-6 border-b border-gray-200">
										<h3 className="text-lg font-medium leading-6 text-gray-900 flex items-center">
											<BarChart3 size={20} className="mr-2" /> Most Recent Data
										</h3>
									</div>
									<div className="px-4 py-5 sm:p-6">
										<div className="grid grid-cols-1 sm:grid-cols-2 gap-6">
											<div>
												<h4 className="text-sm font-medium text-gray-500 mb-2">
													Voltage
												</h4>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.V} V
												</p>
												<div className="mt-1 text-sm text-gray-500">
													{battery.V < voltageThreshold ? (
														<span className="text-amber-600">
															Below threshold ({voltageThreshold}V)
														</span>
													) : (
														<span className="text-green-600">
															Normal operating range
														</span>
													)}
												</div>
											</div>
											<div>
												<h4 className="text-sm font-medium text-gray-500 mb-2">
													Current
												</h4>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.I} A
												</p>
												<div className="mt-1 text-sm text-gray-500">
													{/* {battery.isCharging ? (
                            <span className="text-blue-600">Charging current</span>
                          ) : (
                            <span>Discharge current</span>
                          )} */}
												</div>
											</div>
											<div>
												<h4 className="text-sm font-medium text-gray-500 mb-2">
													Cell 1
												</h4>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.I1} A{" "}
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.T1} °C
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.V1} V
												</p>
											</div>
											<div>
												<h4 className="text-sm font-medium text-gray-500 mb-2">
													Cell 2
												</h4>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.I2} A
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.T2} °C
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.V2} V
												</p>
											</div>
											<div>
												<h4 className="text-sm font-medium text-gray-500 mb-2">
													Cell 3
												</h4>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.I3} A
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.T3} °C
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.V3} V
												</p>
											</div>
											<div>
												<h4 className="text-sm font-medium text-gray-500 mb-2">
													Cell 4
												</h4>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.I4} A
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.T4} °C
												</p>
												<p className="text-2xl font-semibold text-gray-900">
													{battery.V4} V
												</p>
											</div>
										</div>
									</div>
								</div>
							</div>

							{/* Right Column - Actions and Info */}
							<div className="space-y-6">
								<div className="bg-white border border-gray-200 rounded-lg shadow-sm">
									<div className="px-4 py-5 sm:px-6 border-b border-gray-200">
										<h3 className="text-lg font-medium leading-6 text-gray-900">
											Actions
										</h3>
									</div>
									<div className="px-4 py-5 sm:p-6 space-y-4">
										{/* <button
                      onClick={() => onToggleCharging(battery.esp_id)}
                      className={`w-full flex items-center justify-center px-4 py-2 border rounded-md shadow-sm text-sm font-medium focus:outline-none focus:ring-2 focus:ring-offset-2 ${
                        battery.isCharging
                          ? 'border-red-300 text-red-700 bg-red-50 hover:bg-red-100 focus:ring-red-500'
                          : 'border-green-300 text-green-700 bg-green-50 hover:bg-green-100 focus:ring-green-500'
                      }`}
                    >
                      {battery.isCharging ? (
                        <>
                          <Power size={16} className="mr-2" />
                          Stop Charging
                        </>
                      ) : (
                        <>
                          <Zap size={16} className="mr-2" />
                          Start Charging
                        </>
                      )}
                    </button> */}
										<button
											className="w-full flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
											onClick={() =>
												!isFromESP32 ? viewDigitalTwin(battery) : null
											}
										>
											<Layers2 size={16} className="mr-2" />
											Visualisation {isFromESP32 ? "- UNAVAILABLE" : ""}
										</button>
										<button
											onClick={() =>
												isFromESP32 || battery.live_websocket
													? sendUnseal()
													: null
											}
											className="w-full flex items-center justify-center px-4 py-2 border border-orange-300 shadow-sm text-sm font-medium rounded-md text-orange-700 bg-orange-50 hover:bg-orange-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-orange-500"
										>
											<LockKeyholeOpen size={16} className="mr-2" />
											Unseal BMS{" "}
											{!isFromESP32 && !battery.live_websocket
												? "- OFFLINE"
												: ""}
										</button>
										<button
											onClick={() =>
												isFromESP32 || battery.live_websocket
													? sendReset()
													: null
											}
											className="w-full flex items-center justify-center px-4 py-2 border border-red-300 shadow-sm text-sm font-medium rounded-md text-red-700 bg-red-50 hover:bg-red-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500"
										>
											<RefreshCw size={16} className="mr-2" />
											Reset BMS{" "}
											{!isFromESP32 && !battery.live_websocket
												? "- OFFLINE"
												: ""}
										</button>
									</div>
								</div>
							</div>
						</div>

						{/* Beneath */}
						<div className="bg-white border border-gray-200 rounded-lg shadow-sm">
							<div className="px-4 py-5 sm:px-6 border-b border-gray-200">
								<h3 className="text-lg font-medium text-gray-900 flex items-center">
									<Battery size={20} className="mr-2" /> Digital Twin
								</h3>
							</div>
							<div className="p-6">
								{/* <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mb-8">
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Efficiency Rate</h4>
                    <div className="text-2xl font-semibold text-gray-900">94%</div>
                    <p className="mt-1 text-sm text-gray-500">Above average performance</p>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Power Output</h4>
                    <div className="text-2xl font-semibold text-gray-900">2.4 kW</div>
                    <p className="mt-1 text-sm text-gray-500">Nominal output</p>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Temperature Variance</h4>
                    <div className="text-2xl font-semibold text-gray-900">±2.5°C</div>
                    <p className="mt-1 text-sm text-gray-500">Within normal range</p>
                  </div>
                  <div>
                    <h4 className="text-sm font-medium text-gray-500 mb-2">Response Time</h4>
                    <div className="text-2xl font-semibold text-gray-900">50ms</div>
                    <p className="mt-1 text-sm text-gray-500">Optimal performance</p>
                  </div>
                </div> */}

								<div className="space-y-4">
									{!isFromESP32 ? (
										<div>
											<button
												onClick={() =>
													recommendationCards ? null : getRecommendations()
												}
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
																	onClick={() =>
																		implementRecommendation(recommendation)
																	}
																	className="w-auto flex items-center justify-center px-4 py-2 border border-green-300 shadow-sm text-sm font-medium rounded-md text-green-700 bg-green-50 hover:bg-green-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500"
																>
																	{recommendation.implementing === false ? (
																		<>
																			{recommendation.success === true ? (
																				<>Implement</>
																			) : (
																				<>Failed! Try again...</>
																			)}
																			<ArrowBigRightDash
																				size={16}
																				className="ml-2"
																			/>
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
																	onClick={() =>
																		removeRecommendation(recommendation)
																	}
																	className="w-auto flex items-center justify-center px-4 py-2 border border-red-300 shadow-sm text-sm font-medium rounded-md text-red-700 bg-red-50 hover:bg-red-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500"
																>
																	<X size={16} />
																</button>
															</>
														) : (
															<>
																<button
																	onClick={() =>
																		removeRecommendation(recommendation)
																	}
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
									) : (
										<div>
											<h4 className="text-sm font-medium text-gray-500 mb-2">
												Not available offline.
											</h4>
											<h4 className="text-sm font-medium text-gray-500 mb-2">
												Check out our{" "}
												<a
													href="https://batteryportal-e9czhgamgferavf7.ukwest-01.azurewebsites.net/"
													className="text-sm font-medium text-blue-500 mb-2"
												>
													website
												</a>
												!
											</h4>
										</div>
									)}
								</div>
							</div>
						</div>
					</div>
				);
		}
	};

	const navigate = useNavigate();
	const viewDigitalTwin = (battery: BatteryData) => {
		navigate(`/visualisation?esp_id=${battery.esp_id}`);
	};

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
			case "charge-range":
				await processRecommendation(
					recommendation,
					{ Q_low: recommendation.min, Q_high: recommendation.max },
					() =>
						battery.Q_low === recommendation.min &&
						battery.Q_high === recommendation.max,
				);
				break;

			case "current-dischg-limit": {
				await processRecommendation(
					recommendation,
					{ I_dschg_max: recommendation.max },
					() => battery.I_dschg_max === recommendation.max,
				);
				break;
			}

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

	return (
		<div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
			{/* Battery Detail Header */}
			<div className="p-6 border-b border-gray-200 bg-gray-50">
				<div className="flex justify-between items-start">
					<div>
						<h2 className="text-2xl font-bold text-gray-800">
							BMS ID: {battery.esp_id}
						</h2>
					</div>
					<div className="flex items-center space-x-3">
						<span
							className={`px-3 py-1 rounded-full text-sm font-medium ${getStatusColor(isFromESP32 ? true : battery.live_websocket)}`}
						>
							{isFromESP32 || battery.live_websocket ? "online" : "offline"}
						</span>
					</div>
				</div>
			</div>

			{/* Battery Detail Content */}
			<div className="p-6">
				{/* Quick Stats */}
				<div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
					<div className="bg-blue-50 p-4 rounded-lg">
						<div className="flex items-center justify-between mb-2">
							<h3 className="text-sm font-medium text-blue-700 flex items-center">
								{battery.Q > 90 ? (
									<>
										<BatteryFull size={16} className="mr-1" />
									</>
								) : battery.Q > 35 ? (
									<>
										<BatteryMedium size={16} className="mr-1" />
									</>
								) : (
									<>
										<BatteryLow size={16} className="mr-1" />
									</>
								)}{" "}
								Charge Level
							</h3>
							{/* {battery.isCharging && ( */}
							{battery.I > 0 && (
								<span className="bg-yellow-100 text-yellow-800 text-xs px-2 py-1 rounded-full flex items-center">
									<Zap size={12} className="mr-1" /> Charging
								</span>
							)}
						</div>
						<div className="text-3xl font-bold text-blue-700">{battery.Q}%</div>
						<div className="mt-2 bg-blue-200 rounded-full h-2.5">
							<div
								className="h-2.5 rounded-full bg-blue-600"
								style={{ width: `${Math.min(battery.Q, 100)}%` }}
							></div>
						</div>
					</div>

					<div className="bg-orange-50 p-4 rounded-lg">
						<h3 className="text-sm font-medium text-orange-700 flex items-center mb-2">
							<ThermometerSun size={16} className="mr-1" /> Temperature
						</h3>
						<div className="flex items-end space-x-2">
							<span className="text-3xl font-bold text-orange-700">
								Ambient: {battery.cT}°C
							</span>
							{battery.cT > 35 && (
								<span className="text-red-500 text-sm">Above normal</span>
							)}
						</div>
						<div className="flex items-end space-x-2">
							<span className="text-3xl font-bold text-orange-700">
								Cell: {battery.cT}°C
							</span>
							{battery.cT > 35 && (
								<span className="text-red-500 text-sm">Above normal</span>
							)}
						</div>
					</div>

					<div className="bg-green-50 p-4 rounded-lg">
						<h3 className="text-sm font-medium text-green-700 flex items-center mb-2">
							<Info size={16} className="mr-1" /> Battery Health
						</h3>
						<div className="flex items-end space-x-2">
							<span className="text-3xl font-bold text-green-700">
								{battery.H}%
							</span>
							{battery.H < 70 && (
								<span className="text-amber-500 text-sm">Degraded</span>
							)}
						</div>
					</div>

					<div className="bg-purple-50 p-4 rounded-lg">
						<h3 className="text-sm font-medium text-purple-700 flex items-center mb-2">
							{isFromESP32 ? (
								<>
									<Wifi size={16} className="mr-1" /> Wi-Fi
								</>
							) : (
								<>
									<Clock size={16} className="mr-1" /> Most Recent Update
								</>
							)}
						</h3>
						<div className="flex items-end space-x-2">
							<span className="text-3xl font-bold text-purple-700">
								{isFromESP32
									? battery.wifi
										? "Connected"
										: "Not connected"
									: battery.last_updated_time}
							</span>
						</div>
					</div>
				</div>

				{/* Tabs */}
				{/* Tabs */}
				<div className="border-b border-gray-200 mb-6 overflow-x-auto">
					<nav className="-mb-px flex space-x-8">
						<button
							onClick={() => setActiveTab("overview")}
							className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
								activeTab === "overview"
									? "border-blue-500 text-blue-600"
									: "border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300"
							}`}
						>
							Overview
						</button>
						<button
							onClick={() => setActiveTab("settings")}
							className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
								activeTab === "settings"
									? "border-blue-500 text-blue-600"
									: "border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300"
							}`}
						>
							Settings
						</button>
						<button
							onClick={() => setActiveTab("wifi")}
							className={`whitespace-nowrap py-4 px-1 border-b-2 font-medium text-sm ${
								activeTab === "wifi"
									? "border-blue-500 text-blue-600"
									: "border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300"
							}`}
						>
							Wi-Fi
						</button>
					</nav>
				</div>

				{/* Tab Content */}
				{renderTabContent()}
			</div>
		</div>
	);
};

export default BatteryDetail;
