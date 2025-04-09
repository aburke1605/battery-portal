const HTTP_PROTOCAL = window.location.protocol;
const WS_PROTOCAL = HTTP_PROTOCAL == 'https:' ? 'wss' : 'ws';
const HOST = window.location.host;
const HTTP_API_BASE_URL = HTTP_PROTOCAL + '://' + HOST;
const WS_BASE_URL = WS_PROTOCAL + '://' + HOST;

const apiConfig = {
  HTTP_API_BASE_URL: HTTP_API_BASE_URL,
  WS_BASE_URL: WS_BASE_URL,
  WEBSOCKET_BROWSER: import.meta.env.MODE === 'development' ? 'ws://localhost:8888/browser_ws' : `${WS_BASE_URL}/browser_ws`,
};

export default apiConfig;