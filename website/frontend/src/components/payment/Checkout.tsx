import React from "react";
// import ReactDOM from "react-dom/client";
import { loadStripe, Stripe, StripeElements } from "@stripe/stripe-js";
import {
  PaymentElement,
  Elements,
  ElementsConsumer,
} from "@stripe/react-stripe-js";
import apiConfig from "../../apiConfig";

// Define prop types
interface CheckoutFormProps {
  stripe: Stripe | null;
  elements: StripeElements | null;
}

class CheckoutForm extends React.Component<CheckoutFormProps> {
  handleSubmit = async (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    const { stripe, elements } = this.props;

    if (!stripe || !elements) return;

    const { error: submitError } = await elements.submit();
    if (submitError) {
      console.error(submitError.message);
      return;
    }

    const res = await fetch("/api/pay/create-intent", { method: "POST" });
    const { client_secret: clientSecret } = await res.json();

    const { error } = await stripe.confirmPayment({
      elements,
      clientSecret,
      confirmParams: { return_url: "https://example.com/order/123/complete" },
    });

    if (error) console.error(error.message);
  };

  render() {
    const { stripe } = this.props;
    return (
      <form onSubmit={this.handleSubmit}>
        <PaymentElement />
        <button type="submit" disabled={!stripe}>
          Pay
        </button>
      </form>
    );
  }
}

// const InjectedCheckoutForm = () => (
//   <ElementsConsumer>
//     {({ stripe, elements }) => (
//       <CheckoutForm stripe={stripe} elements={elements} />
//     )}
//   </ElementsConsumer>
// );

const stripePromise = loadStripe(apiConfig.PAY_PUBLIC_KEY);

const options = {
  mode: "payment" as const,
  amount: 1099,
  currency: "usd",
  appearance: {},
};

// const App = () => (
//   <Elements stripe={stripePromise} options={options}>
//     <InjectedCheckoutForm />
//   </Elements>
// );

// // React 18+ mount
// const root = ReactDOM.createRoot(document.getElementById("root")!);
// root.render(<App />);

export default function CheckoutOverlay({ open, onClose }: { open: boolean; onClose: () => void }) {
  if (!open) return null;

  return (
    <div className="fixed inset-0 bg-black/50 flex items-center justify-center">
      <div className="bg-white p-6 rounded shadow-md w-[400px] relative">
        <button className="absolute top-2 right-2" onClick={onClose}>✕</button>
        <Elements stripe={stripePromise} options={options}>
          <ElementsConsumer>
            {({ stripe, elements }) => (
              <CheckoutForm stripe={stripe} elements={elements} />
            )}
          </ElementsConsumer>
        </Elements>
      </div>
    </div>
  );
}