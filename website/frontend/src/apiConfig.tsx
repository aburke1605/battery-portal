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
  WEBSOCKET_BROWSER: import.meta.env.MODE === 'development' ? 'ws://localhost:8888/browser_ws' : `${WS_BASE_URL}/api/browser_ws`,
  USER_API: `${HTTP_PROTOCOL}//${HOST}/api/user`,
  DB_EXECUTE_SQL_API: `${HTTP_PROTOCOL}//${HOST}/api/db/execute_sql`,
  DB_INFO_API: `${HTTP_PROTOCOL}//${HOST}/api/db/info`,
  DB_ESP_ID_API: `${HTTP_PROTOCOL}//${HOST}/api/db/esp_ids`,
  DB_DATA_API: `${HTTP_PROTOCOL}//${HOST}/api/db/data`,
  DB_CHART_DATA_API: `${HTTP_PROTOCOL}//${HOST}/api/db/chart_data`,
  DB_RECOMMENDATION_API: `${HTTP_PROTOCOL}//${HOST}/api/db/recommendation`,
};

export default apiConfig;