import { useEffect, useState } from "react";
import PageMeta from "../components/common/PageMeta";
import PageBreadcrumb from "../components/common/PageBreadCrumb";
import StripeButton from "../components/payment/Checkout";
import { fromAuthenticator } from "../auth/UserAuthenticator";

export default function SubscriptionManagement() {
  const { user, fetchUserData } = fromAuthenticator();
  const [isSubscribed, setIsSubscribed] = useState<boolean>(false);
  const [expiryDate, setExpiryDate] = useState<string | null>(null);

  useEffect(() => {
    if (!user?.email) return;

    const loadSubscriptionStatus = async () => {
      await fetchUserData();
      setIsSubscribed(user.subscribed);
      setExpiryDate(user.subscription_expiry);
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
                    One month £9.99 (£9.99 / month)
                    <StripeButton price={9.99} length={1} />
                  </div>
                  <div>
                    One year £99.99 (£8.33 / month)
                    <StripeButton price={99.99} length={12} />
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
