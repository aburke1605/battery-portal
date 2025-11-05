import React, { useState, useEffect } from "react";
import {
	PaymentElement,
	useStripe,
	useElements,
	Elements,
} from "@stripe/react-stripe-js";
import { loadStripe } from "@stripe/stripe-js";
import apiConfig from "../../apiConfig";

const stripePromise = loadStripe(apiConfig.PAY_PUBLIC_KEY);

function CheckoutForm({ onClose }: { onClose: () => void }) {
	const stripe = useStripe();
	const elements = useElements();
	const [loading, setLoading] = useState(false);
	const [message, setMessage] = useState("");

	const handleSubmit = async (e: React.FormEvent) => {
		e.preventDefault();
		if (!stripe || !elements) return;

		setLoading(true);
		setMessage("");

		const { error } = await stripe.confirmPayment({
			elements,
			confirmParams: { return_url: "http://localhost:5173/success" },
		});

		if (error) setMessage(error.message ?? "Payment failed");
		setLoading(false);
	};

	return (
		<div
			style={{
				position: "fixed",
				top: 0,
				left: 0,
				width: "100vw",
				height: "100vh",
				backgroundColor: "rgba(0,0,0,0.6)",
				display: "flex",
				justifyContent: "center",
				alignItems: "center",
				zIndex: 1000,
			}}
		>
			<div
				style={{
					backgroundColor: "white",
					padding: 20,
					borderRadius: 8,
					width: 400,
					maxWidth: "90%",
					position: "relative",
				}}
			>
				<button
					onClick={onClose}
					style={{
						position: "absolute",
						top: 10,
						right: 10,
						background: "none",
						border: "none",
						fontSize: 18,
						cursor: "pointer",
					}}
				>
					✕
				</button>

				<h2 style={{ textAlign: "center" }}>Pay $10.00</h2>

				<form onSubmit={handleSubmit} style={{ marginTop: 20 }}>
					<PaymentElement />
					<button
						type="submit"
						disabled={!stripe || loading}
						style={{
							marginTop: 20,
							width: "100%",
							padding: 10,
							backgroundColor: "#0070f3",
							color: "white",
							border: "none",
							borderRadius: 4,
							cursor: "pointer",
						}}
					>
						{loading ? "Processing..." : "Pay now"}
					</button>
					{message && (
						<p style={{ color: "red", marginTop: 10, fontSize: 14 }}>
							{message}
						</p>
					)}
				</form>
			</div>
		</div>
	);
}

export default function StripeButton() {
	const [showButton, setShowButton] = useState(false);
	const [clientSecret, setClientSecret] = useState<string | null>(null);
	const [loadingIntent, setLoadingIntent] = useState(false);
	const [error, setError] = useState<string | null>(null);

	useEffect(() => {
		if (showButton) {
			setLoadingIntent(true);
			setError(null);

			fetch("/api/pay/create-payment-intent", {
				method: "POST",
				headers: { "Content-Type": "application/json" },
				body: JSON.stringify({ amount: 10.0 }),
			})
				.then(async (res) => {
					if (!res.ok) throw new Error(`Server returned ${res.status}`);
					const data = await res.json();
					if (!data.clientSecret) throw new Error("No clientSecret returned");
					setClientSecret(data.clientSecret);
					console.log("SUCCESS");
				})
				.catch((err: Error) => setError(err.message))
				.finally(() => setLoadingIntent(false));
		} else {
			setClientSecret(null);
		}
	}, [showButton]);

	const options = clientSecret ? { clientSecret } : undefined;

	return (
		<>
			<button
				onClick={() => setShowButton(true)}
				style={{ padding: "10px 20px", cursor: "pointer" }}
			>
				Pay $10
			</button>

			{showButton && (
				<>
					{loadingIntent && (
						<div
							style={{
								position: "fixed",
								top: 0,
								left: 0,
								width: "100vw",
								height: "100vh",
								backgroundColor: "rgba(0,0,0,0.6)",
								display: "flex",
								justifyContent: "center",
								alignItems: "center",
								zIndex: 1000,
								color: "white",
								fontSize: 18,
							}}
						>
							Loading payment form...
						</div>
					)}

					{error && (
						<div
							style={{
								position: "fixed",
								top: 0,
								left: 0,
								width: "100vw",
								height: "100vh",
								backgroundColor: "rgba(255,0,0,0.8)",
								display: "flex",
								justifyContent: "center",
								alignItems: "center",
								zIndex: 1000,
								color: "white",
								padding: 20,
							}}
						>
							<div style={{ textAlign: "center" }}>
								<p>Error: {error}</p>
								<button
									onClick={() => setShowButton(false)}
									style={{
										marginTop: 10,
										padding: "8px 16px",
										cursor: "pointer",
									}}
								>
									Close
								</button>
							</div>
						</div>
					)}

					{clientSecret && options && (
						<Elements stripe={stripePromise} options={options}>
							<CheckoutForm onClose={() => setShowButton(false)} />
						</Elements>
					)}
				</>
			)}
		</>
	);
}
