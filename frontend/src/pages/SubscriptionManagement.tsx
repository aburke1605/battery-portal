import { useEffect, useState } from "react";
import { useAuth } from "../auth/AuthContext";
import axios from "axios";
import apiConfig from "../apiConfig";

type SubscriptionStatus = {
  subscribed: boolean;
  expiryDate: string | null;
};

async function getSubscriptionStatus(
  email: string | undefined,
): Promise<SubscriptionStatus> {
  if (!email) return { subscribed: false, expiryDate: null };

  try {
    if (email === "user@admin.dev")
      return { subscribed: true, expiryDate: "Never" };

    const resp = await axios.get(
      `${apiConfig.USER_API}/subscription?email=${email}`,
    );
    if (resp.data.status == "success") {
      return {
        subscribed: resp.data.subscribed,
        expiryDate: resp.data.expiryDate ?? null,
      };
    }
    return { subscribed: false, expiryDate: null };
  } catch (error) {
    console.log("Error fetching subscription data:", error);
    return { subscribed: false, expiryDate: null };
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
      setExpiryDate(subscriptionStatus.expiryDate);
    };
    loadSubscriptionStatus();
  }, [user?.email]);

  return (
    <>
      <div>
        Hi, {user?.first_name}.<br></br>
        {isSubscribed ? <>Subscribed</> : <>Not subscribed</>}
      </div>
    </>
  );
}
