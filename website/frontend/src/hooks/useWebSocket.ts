import { useEffect, useRef, useCallback } from "react";

export function useWebSocket({
  url,
  onMessage,
  autoConnect = true,
}: {
  url: string;
  onMessage: (data: any) => void;
  autoConnect?: boolean;
}) {
  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    if (!autoConnect) return;

    const socket = new WebSocket(url);
    ws.current = socket;

    socket.onopen = () => console.log("WebSocket connected:", url);
    socket.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        onMessage(data);
      } catch (e) {
        console.error("Invalid JSON from WebSocket", e);
      }
    };
    socket.onclose = () => console.log("WebSocket disconnected:", url);
    socket.onerror = (e) => console.error("WebSocket error:", e);

    return () => socket.close();
  }, [url, autoConnect, onMessage]);

  const sendMessage = useCallback((msg: any) => {
    if (ws.current?.readyState === WebSocket.OPEN) {
      const payload = JSON.stringify(msg);
      ws.current.send(payload);
      console.log("Sent:", payload);
    } else {
      console.warn("WebSocket not connected");
    }
  }, []);

  return { sendMessage };
}



export const createMessage = (
  summary: string,
  data: any,
  esp_id?: string
) => ({
  type: "request",
  content: {
    summary,
    data: esp_id ? { esp_id, ...data } : data,
  },
});
