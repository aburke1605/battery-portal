#define AP_WIFI_SSID "AceOn battery"
#define AP_WIFI_PASS "password"
#define AP_MAX_STA_CONN 4

extern int other_AP_SSIDs[256];

void wifi_scan(void);

int find_unique_SSID(void);

void wifi_init(void);
