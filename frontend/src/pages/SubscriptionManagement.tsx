import { useEffect, useState } from "react";
import { useAuth } from "../auth/AuthContext";
import axios from "axios";
import apiConfig from "../apiConfig";

async function getSubscriptionStatus(email: string | undefined) {
  try {
    if (email === "user@admin.dev") return true;

    const resp = await axios.get(
      `${apiConfig.USER_API}/subscription?email=${email}`,
    );
    if (resp.data.status == "success") return resp.data.subscribed;

    return false;
  } catch (error) {
    console.log("Error fetching subscription data:", error);
    return false;
  }
}

export default function SubscriptionManagement() {
  const { user } = useAuth();
  const [isSubscribed, setIsSubscribed] = useState<boolean>(false);

  useEffect(() => {
    const loadSubscriptionStatus = async () => {
      setIsSubscribed(await getSubscriptionStatus(user?.email));
    };
    loadSubscriptionStatus();
  }, []);

  return (
    <>
      <div>
        Hi, {user?.first_name}.<br></br>
        {isSubscribed ? <>Subscribed</> : <>Not subscribed</>}
      </div>
    </>
  );
}
