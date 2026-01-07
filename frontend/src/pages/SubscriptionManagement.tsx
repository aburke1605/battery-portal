import { useEffect, useState } from "react";
import { useAuth } from "../auth/AuthContext";
import axios from "axios";
import apiConfig from "../apiConfig";
import PageMeta from "../components/common/PageMeta";
import PageBreadcrumb from "../components/common/PageBreadCrumb";
import StripeButton from "../components/payment/Checkout";

type SubscriptionStatus = {
  subscribed: boolean;
  expiry: string | null;
};

export async function getSubscriptionStatus(
  email: string | undefined,
): Promise<SubscriptionStatus> {
  if (!email) return { subscribed: false, expiry: null };

  try {
    if (email === "user@admin.dev")
      return { subscribed: true, expiry: "Never" };

    const resp = await axios.get(
      `${apiConfig.USER_API}/subscription?email=${email}`,
    );
    if (resp.data.status == "success") {
      return {
        subscribed: resp.data.subscribed,
        expiry: resp.data.expiry ?? null,
      };
    }
    return { subscribed: false, expiry: null };
  } catch (error) {
    console.log("Error fetching subscription data:", error);
    return { subscribed: false, expiry: null };
  }
}

export default function SubscriptionManagement() {
  const { user } = useAuth();
  const [isSubscribed, setIsSubscribed] = useState<boolean>(false);
  const [expiryDate, setExpiryDate] = useState<string | null>(null);

  useEffect(() => {
    if (!user?.email) return;

    const loadSubscriptionStatus = async () => {
      let subscriptionStatus = await getSubscriptionStatus(user?.email);
      setIsSubscribed(subscriptionStatus.subscribed);
      setExpiryDate(subscriptionStatus.expiry);
    };
    loadSubscriptionStatus();
  }, [user]);

  return (
    <>
      <PageMeta
        title="Subscription Dashboard"
        description="React.js Subscription Dashboard Tailwind CSS Admin"
      />
      <PageBreadcrumb pageTitle="Subscription management" />

      <div className="space-y-6">
        <div className="bg-white dark:bg-gray-900 rounded-xl shadow-sm border border-gray-200 dark:border-gray-700">
          <div className="px-6 py-4 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-3">
                <div>
                  <h3 className="text-lg font-semibold text-gray-900 dark:text-white">
                    Hi, {user?.first_name}.
                  </h3>
                  <p className="text-sm text-gray-500 dark:text-gray-400">
                    Manage your account subscription
                  </p>
                </div>
              </div>
            </div>
          </div>

          <div className="p-6">
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              {isSubscribed ? (
                <>
                  <div>Your subscription expiry: {expiryDate}</div>
                </>
              ) : (
                <>
                  <div>
                    Subscribe now
                    <StripeButton price={9.99} />
                  </div>
                </>
              )}
            </div>
          </div>
        </div>
      </div>
    </>
  );
}
