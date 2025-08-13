const HTTP_PROTOCOL = window.location.protocol;
const WS_PROTOCOL = HTTP_PROTOCOL == 'https:' ? 'wss' : 'ws';
const HOST = window.location.host;
const HTTP_API_BASE_URL = HTTP_PROTOCOL + '://' + HOST;
const WS_BASE_URL = WS_PROTOCOL + '://' + HOST;

const apiConfig = {
  ADMIN_USERNAME: "admin",
  ADMIN_PASSWORD: "BaTT3ryP0rta!",
  HTTP_API_BASE_URL: HTTP_API_BASE_URL,
  WS_BASE_URL: WS_BASE_URL,
  WEBSOCKET_BROWSER: import.meta.env.MODE === 'development' ? 'ws://localhost:8888/browser_ws' : `${WS_BASE_URL}/browser_ws`,
  DB_ID_END_POINT: `${HTTP_PROTOCOL}//${HOST}/db/ids`,
  DB_DATA_END_POINT: `${HTTP_PROTOCOL}//${HOST}/db/data`,
};

export default apiConfig;