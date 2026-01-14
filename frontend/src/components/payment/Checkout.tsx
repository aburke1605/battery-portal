import React, { useState, useEffect } from "react";
import {
  PaymentElement,
  useStripe,
  useElements,
  Elements,
} from "@stripe/react-stripe-js";
import { loadStripe } from "@stripe/stripe-js";
import apiConfig from "../../apiConfig";
import { fromAuthenticator } from "../../auth/UserAuthenticator";
import axios from "axios";

const stripePromise = loadStripe(apiConfig.PAY_PUBLIC_KEY);

function CheckoutForm({
  price,
  onClose,
}: { price: number; onClose: () => void }) {
  const stripe = useStripe();
  const elements = useElements();
  const [loading, setLoading] = useState(false);
  const [message, setMessage] = useState("");

  const { fetchUserData } = fromAuthenticator();

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!stripe || !elements) return;

    setLoading(true);
    setMessage("");

    const { error } = await stripe.confirmPayment({
      elements,
      redirect: "if_required",
    });

    if (error) {
      setMessage(error.message ?? "Payment failed");
      setLoading(false);
      return;
    }

    await fetchUserData();

    onClose();
  };

  return (
    <div className="fixed inset-0 bg-black/60 flex justify-center items-center z-[1000]">
      <div className="bg-white p-5 rounded-lg w-96 max-w-[90%] relative">
        <button
          onClick={onClose}
          className="absolute top-2.5 right-2.5 text-lg bg-none border-none cursor-pointer"
        >
          ✕
        </button>

        <h2 className="text-center text-xl font-semibold">
          £{price.toFixed(2)}
        </h2>

        <form onSubmit={handleSubmit} className="mt-5">
          <PaymentElement />
          <button
            type="submit"
            disabled={!stripe || loading}
            className="w-full flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 mt-4 mb-4"
          >
            {loading ? "Processing..." : "Pay now"}
          </button>
          {message && <p className="text-red-500 mt-2 text-sm">{message}</p>}
        </form>
      </div>
    </div>
  );
}

export default function StripeButton({ price }: { price: number }) {
  const [showCheckout, setShowCheckout] = useState(false);
  const [clientSecret, setClientSecret] = useState<string | null>(null);
  const [loadingIntent, setLoadingIntent] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const { user } = fromAuthenticator();

  useEffect(() => {
    if (showCheckout) {
      setLoadingIntent(true);
      setError(null);

      axios
        .post(apiConfig.PAY_INTENT_API, {
          amount: price,
          email: user?.email,
          length: length,
        })
        .then((response) => {
          const data = response.data;
          if (!data?.clientSecret) {
            throw new Error("No clientSecret returned");
          }
          setClientSecret(data.clientSecret);
        })
        .catch((error) => {
          setError(
            axios.isAxiosError(error)
              ? (error.response?.data?.message ?? error.message)
              : String(error),
          );
        })
        .finally(() => setLoadingIntent(false));
    } else {
      setClientSecret(null);
    }
  }, [showCheckout]);

  const options = clientSecret ? { clientSecret } : undefined;

  return (
    <>
      <button
        onClick={() => setShowCheckout(true)}
        className="w-full flex items-center justify-center px-4 py-2 border border-blue-300 shadow-sm text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 mt-4 mb-4"
      >
        Pay £{price.toFixed(2)}
      </button>

      {showCheckout && (
        <>
          {loadingIntent && (
            <div className="fixed inset-0 bg-black/60 flex justify-center items-center z-[1000] text-white text-lg">
              Loading payment form...
            </div>
          )}

          {error && (
            <div className="fixed inset-0 bg-red-800/80 flex justify-center items-center z-[1000] p-5">
              <div className="text-center text-white">
                <p>Error: {error}</p>
                <button
                  onClick={() => setShowCheckout(false)}
                  className="mt-2 px-4 py-2 border rounded-md cursor-pointer bg-white text-red-800 hover:bg-red-100"
                >
                  Close
                </button>
              </div>
            </div>
          )}

          {clientSecret && options && (
            <Elements stripe={stripePromise} options={options}>
              <CheckoutForm
                price={price}
                onClose={() => setShowCheckout(false)}
              />
            </Elements>
          )}
        </>
      )}
    </>
  );
}
