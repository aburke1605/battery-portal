import { useState } from "react";
import { useAuth } from "../auth/AuthContext";

export default function SubscriptionManagement() {
  const { user } = useAuth();
  const [isSubscribed, setIsSubscribed] = useState<boolean>(false);

  return (
    <>
      <div>
        Hi, {user?.first_name}.<br></br>
        {isSubscribed ? <>Subscribed</> : <>Not subscribed</>}
      </div>
    </>
  );
}
