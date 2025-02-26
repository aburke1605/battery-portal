#define WIFI_SSID "AceOn battery"
#define WIFI_PASS "password"
#define MAX_STA_CONN 4

extern int other_AP_SSIDs[256];
extern char ESP_subnet_IP[15];

void wifi_scan(void);

int find_unique_SSID(void);

void wifi_init(void);
