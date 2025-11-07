import { useState, useEffect } from "react";
import PageBreadcrumb from "../components/common/PageBreadCrumb";
import PageMeta from "../components/common/PageMeta";
import { useAuth } from "../auth/AuthContext";
import Button from "../components/ui/button/Button";
import Input from "../components/form/input/InputField";
import Label from "../components/form/Label";
import {
	User,
	Mail,
	Lock,
	Edit3,
	Save,
	X,
	AlertTriangle,
	CheckCircle,
} from "lucide-react";
import apiConfig from "../apiConfig";
import axios from "axios";

interface ProfileFormData {
	first_name: string;
	last_name: string;
	email: string;
}

interface PasswordFormData {
	current_password: string;
	new_password: string;
	confirm_password: string;
}

export default function UserProfiles() {
	const { user } = useAuth();
	const [isEditing, setIsEditing] = useState<boolean>(false);
	const [isChangingPassword, setIsChangingPassword] = useState<boolean>(false);
	const [loading, setLoading] = useState<boolean>(false);
	const [profileData, setProfileData] = useState<ProfileFormData>({
		first_name: "",
		last_name: "",
		email: "",
	});
	const [passwordData, setPasswordData] = useState<PasswordFormData>({
		current_password: "",
		new_password: "",
		confirm_password: "",
	});
	const [errors, setErrors] = useState<{ [key: string]: string }>({});
	const [successMessage, setSuccessMessage] = useState<string>("");

	// Load user data when component mounts or user changes
	useEffect(() => {
		if (user) {
			setProfileData({
				first_name: user.first_name || "",
				last_name: user.last_name || "",
				email: user.email || "",
			});
		}
	}, [user]);

	const validateProfileForm = () => {
		const newErrors: { [key: string]: string } = {};

		if (!profileData.first_name.trim()) {
			newErrors.first_name = "First name is required";
		}

		if (!profileData.last_name.trim()) {
			newErrors.last_name = "Last name is required";
		}

		if (!profileData.email.trim()) {
			newErrors.email = "Email is required";
		} else if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(profileData.email)) {
			newErrors.email = "Please enter a valid email address";
		}

		setErrors(newErrors);
		return Object.keys(newErrors).length === 0;
	};

	const validatePasswordForm = () => {
		const newErrors: { [key: string]: string } = {};

		if (!passwordData.current_password) {
			newErrors.current_password = "Current password is required";
		}

		if (!passwordData.new_password) {
			newErrors.new_password = "New password is required";
		} else if (passwordData.new_password.length < 6) {
			newErrors.new_password = "Password must be at least 6 characters long";
		}

		if (!passwordData.confirm_password) {
			newErrors.confirm_password = "Please confirm your new password";
		} else if (passwordData.new_password !== passwordData.confirm_password) {
			newErrors.confirm_password = "Passwords do not match";
		}

		setErrors(newErrors);
		return Object.keys(newErrors).length === 0;
	};

	const handleProfileInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
		const { name, value } = e.target;
		setProfileData((prev) => ({ ...prev, [name]: value }));

		// Clear error for this field when user starts typing
		if (errors[name]) {
			setErrors((prev) => ({ ...prev, [name]: "" }));
		}
	};

	const handlePasswordInputChange = (
		e: React.ChangeEvent<HTMLInputElement>,
	) => {
		const { name, value } = e.target;
		setPasswordData((prev) => ({ ...prev, [name]: value }));

		// Clear error for this field when user starts typing
		if (errors[name]) {
			setErrors((prev) => ({ ...prev, [name]: "" }));
		}
	};

	const handleSaveProfile = async () => {
		if (!validateProfileForm()) return;

		setLoading(true);
		setSuccessMessage("");

		let unknown_error = true; // assume unknown error, change if error known or no error
		try {
			const res = await axios.put(`${apiConfig.USER_API}/profile`, profileData);

			let message = res.data.success;
			if (message) {
				setIsEditing(false);
				setSuccessMessage(message);
				unknown_error = false;
			}
		} catch (err) {
			if (axios.isAxiosError(err)) {
				let res = err.response;
				if (res) {
					let message = res.data.error;
					if (message) {
						if (message.toLowerCase().includes("email"))
							setErrors({ email: message });
						else setErrors({ general: message });
						unknown_error = false;
					}
				}
			}
		} finally {
			setLoading(false);
		}
		if (unknown_error) setErrors({ general: "Unknown error occurred" });
	};

	const handleChangePassword = async () => {
		if (!validatePasswordForm()) return;

		setLoading(true);
		setSuccessMessage("");

		let unknown_error = true; // assume unknown error, change if error known or no error
		try {
			const res = await axios.put(`${apiConfig.USER_API}/change-password`, {
				current_password: passwordData.current_password,
				new_password: passwordData.new_password,
			});

			let message = res.data.success;
			if (message) {
				setIsChangingPassword(false);
				setPasswordData({
					current_password: "",
					new_password: "",
					confirm_password: "",
				});
				setSuccessMessage(message);
				unknown_error = false;
			}
		} catch (err) {
			if (axios.isAxiosError(err)) {
				let res = err.response;
				if (res) {
					let message = res.data.error;
					if (message) {
						setErrors({ general: message });
						if (message.toLowerCase().includes("password")) {
							if (message.toLowerCase().includes("current"))
								setErrors({ current_password: message });
							else if (message.toLowerCase().includes("new"))
								setErrors({ new_password: message });
						}
						unknown_error = false;
					}
				}
			}
		} finally {
			setLoading(false);
		}
		if (unknown_error) setErrors({ general: "Unknown error occurred" });
	};

	const handleCancelEdit = () => {
		setIsEditing(false);
		setIsChangingPassword(false);
		setErrors({});
		setSuccessMessage("");

		// Reset profile data
		if (user) {
			setProfileData({
				first_name: user.first_name || "",
				last_name: user.last_name || "",
				email: user.email || "",
			});
		}

		// Reset password data
		setPasswordData({
			current_password: "",
			new_password: "",
			confirm_password: "",
		});
	};

	return (
		<>
			<PageMeta
				title="Profile Dashboard"
				description="React.js Profile Dashboard Tailwind CSS Admin"
			/>
			<PageBreadcrumb pageTitle="Profile" />

			<div className="space-y-6">
				{/* Success Message */}
				{successMessage && (
					<div className="p-4 bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 rounded-lg">
						<div className="flex items-start">
							<CheckCircle className="w-5 h-5 text-green-600 dark:text-green-400 mt-0.5 mr-3 flex-shrink-0" />
							<div>
								<h4 className="text-sm font-medium text-green-800 dark:text-green-200">
									Success
								</h4>
								<p className="mt-1 text-sm text-green-700 dark:text-green-300">
									{successMessage}
								</p>
							</div>
						</div>
					</div>
				)}

				{/* Error Messages */}
				{(errors.general || Object.keys(errors).length > 0) && (
					<div className="p-4 bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 rounded-lg">
						<div className="flex items-start">
							<AlertTriangle className="w-5 h-5 text-red-600 dark:text-red-400 mt-0.5 mr-3 flex-shrink-0" />
							<div>
								<h4 className="text-sm font-medium text-red-800 dark:text-red-200">
									{errors.general
										? "Error"
										: "Please fix the following errors:"}
								</h4>
								{errors.general ? (
									<p className="mt-2 text-sm text-red-700 dark:text-red-300">
										{errors.general}
									</p>
								) : (
									<ul className="mt-2 text-sm text-red-700 dark:text-red-300 list-disc list-inside space-y-1">
										{Object.entries(errors)
											.filter(([key]) => key !== "general")
											.map(([, error], index) => (
												<li key={index}>{error}</li>
											))}
									</ul>
								)}
							</div>
						</div>
					</div>
				)}

				{/* Profile Information Card */}
				<div className="bg-white dark:bg-gray-900 rounded-xl shadow-sm border border-gray-200 dark:border-gray-700">
					<div className="px-6 py-4 border-b border-gray-200 dark:border-gray-700">
						<div className="flex items-center justify-between">
							<div className="flex items-center gap-3">
								<div className="p-2 bg-blue-100 dark:bg-blue-900/20 rounded-lg">
									<User className="w-6 h-6 text-blue-600 dark:text-blue-400" />
								</div>
								<div>
									<h3 className="text-lg font-semibold text-gray-900 dark:text-white">
										Profile Information
									</h3>
									<p className="text-sm text-gray-500 dark:text-gray-400">
										Update your account's profile information and email address
									</p>
								</div>
							</div>
							{!isEditing && !isChangingPassword && (
								<Button
									size="sm"
									onClick={() => setIsEditing(true)}
									className="bg-blue-600 hover:bg-blue-700 text-white"
								>
									<Edit3 className="w-4 h-4 mr-2" />
									Edit Profile
								</Button>
							)}
						</div>
					</div>

					<div className="p-6">
						<div className="grid grid-cols-1 md:grid-cols-2 gap-6">
							<div>
								<Label>First Name</Label>
								{isEditing ? (
									<Input
										name="first_name"
										type="text"
										value={profileData.first_name}
										onChange={handleProfileInputChange}
										placeholder="Enter first name"
										className={`mt-1 ${errors.first_name ? "border-red-500 focus:ring-red-500" : ""}`}
									/>
								) : (
									<div className="mt-1 px-3 py-2 bg-gray-50 dark:bg-gray-800 rounded-lg text-gray-900 dark:text-white">
										{profileData.first_name || "Not set"}
									</div>
								)}
								{errors.first_name && (
									<p className="mt-1 text-sm text-red-600 dark:text-red-400">
										{errors.first_name}
									</p>
								)}
							</div>

							<div>
								<Label>Last Name</Label>
								{isEditing ? (
									<Input
										name="last_name"
										type="text"
										value={profileData.last_name}
										onChange={handleProfileInputChange}
										placeholder="Enter last name"
										className={`mt-1 ${errors.last_name ? "border-red-500 focus:ring-red-500" : ""}`}
									/>
								) : (
									<div className="mt-1 px-3 py-2 bg-gray-50 dark:bg-gray-800 rounded-lg text-gray-900 dark:text-white">
										{profileData.last_name || "Not set"}
									</div>
								)}
								{errors.last_name && (
									<p className="mt-1 text-sm text-red-600 dark:text-red-400">
										{errors.last_name}
									</p>
								)}
							</div>

							<div className="md:col-span-2">
								<Label>Email Address</Label>
								{isEditing ? (
									<Input
										name="email"
										type="email"
										value={profileData.email}
										onChange={handleProfileInputChange}
										placeholder="Enter email address"
										className={`mt-1 ${errors.email ? "border-red-500 focus:ring-red-500" : ""}`}
									/>
								) : (
									<div className="mt-1 px-3 py-2 bg-gray-50 dark:bg-gray-800 rounded-lg text-gray-900 dark:text-white flex items-center">
										<Mail className="w-4 h-4 mr-2 text-gray-400" />
										{profileData.email || "Not set"}
									</div>
								)}
								{errors.email && (
									<p className="mt-1 text-sm text-red-600 dark:text-red-400">
										{errors.email}
									</p>
								)}
							</div>
						</div>

						{isEditing && (
							<div className="flex items-center justify-end gap-3 mt-6 pt-6 border-t border-gray-200 dark:border-gray-700">
								<Button
									size="sm"
									variant="outline"
									onClick={handleCancelEdit}
									disabled={loading}
								>
									<X className="w-4 h-4 mr-2" />
									Cancel
								</Button>
								<Button
									size="sm"
									onClick={handleSaveProfile}
									disabled={loading}
									className="bg-blue-600 hover:bg-blue-700 text-white"
								>
									<Save className="w-4 h-4 mr-2" />
									{loading ? "Saving..." : "Save Changes"}
								</Button>
							</div>
						)}
					</div>
				</div>

				{/* Password Change Card */}
				<div className="bg-white dark:bg-gray-900 rounded-xl shadow-sm border border-gray-200 dark:border-gray-700">
					<div className="px-6 py-4 border-b border-gray-200 dark:border-gray-700">
						<div className="flex items-center justify-between">
							<div className="flex items-center gap-3">
								<div className="p-2 bg-orange-100 dark:bg-orange-900/20 rounded-lg">
									<Lock className="w-6 h-6 text-orange-600 dark:text-orange-400" />
								</div>
								<div>
									<h3 className="text-lg font-semibold text-gray-900 dark:text-white">
										Password
									</h3>
									<p className="text-sm text-gray-500 dark:text-gray-400">
										Update your password to keep your account secure
									</p>
								</div>
							</div>
							{!isChangingPassword && !isEditing && (
								<Button
									size="sm"
									onClick={() => setIsChangingPassword(true)}
									className="bg-orange-600 hover:bg-orange-700 text-white"
								>
									<Lock className="w-4 h-4 mr-2" />
									Change Password
								</Button>
							)}
						</div>
					</div>

					<div className="p-6">
						{isChangingPassword ? (
							<div className="space-y-4">
								<div>
									<Label>Current Password</Label>
									<Input
										name="current_password"
										type="password"
										value={passwordData.current_password}
										onChange={handlePasswordInputChange}
										placeholder="Enter current password"
										className={`mt-1 ${errors.current_password ? "border-red-500 focus:ring-red-500" : ""}`}
									/>
									{errors.current_password && (
										<p className="mt-1 text-sm text-red-600 dark:text-red-400">
											{errors.current_password}
										</p>
									)}
								</div>

								<div>
									<Label>New Password</Label>
									<Input
										name="new_password"
										type="password"
										value={passwordData.new_password}
										onChange={handlePasswordInputChange}
										placeholder="Enter new password"
										className={`mt-1 ${errors.new_password ? "border-red-500 focus:ring-red-500" : ""}`}
									/>
									{errors.new_password && (
										<p className="mt-1 text-sm text-red-600 dark:text-red-400">
											{errors.new_password}
										</p>
									)}
								</div>

								<div>
									<Label>Confirm New Password</Label>
									<Input
										name="confirm_password"
										type="password"
										value={passwordData.confirm_password}
										onChange={handlePasswordInputChange}
										placeholder="Confirm new password"
										className={`mt-1 ${errors.confirm_password ? "border-red-500 focus:ring-red-500" : ""}`}
									/>
									{errors.confirm_password && (
										<p className="mt-1 text-sm text-red-600 dark:text-red-400">
											{errors.confirm_password}
										</p>
									)}
								</div>

								<div className="flex items-center justify-end gap-3 pt-4 border-t border-gray-200 dark:border-gray-700">
									<Button
										size="sm"
										variant="outline"
										onClick={handleCancelEdit}
										disabled={loading}
									>
										<X className="w-4 h-4 mr-2" />
										Cancel
									</Button>
									<Button
										size="sm"
										onClick={handleChangePassword}
										disabled={loading}
										className="bg-orange-600 hover:bg-orange-700 text-white"
									>
										<Save className="w-4 h-4 mr-2" />
										{loading ? "Changing..." : "Change Password"}
									</Button>
								</div>
							</div>
						) : (
							<div className="text-center py-8">
								<Lock className="w-12 h-12 text-gray-400 mx-auto mb-4" />
								<p className="text-gray-500 dark:text-gray-400 mb-4">
									Your password is securely encrypted and hidden for your
									protection.
								</p>
								<p className="text-sm text-gray-400 dark:text-gray-500">
									Last changed: Never or information not available
								</p>
							</div>
						)}
					</div>
				</div>
			</div>
		</>
	);
}
