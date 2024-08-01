#define CONNECT (1 << 0)
#define FAIL    (1 << 1)
#define C       (1 << 2)
#define SSID    ""
#define PASS    ""


extern EventGroupHandle_t wifi_event;
extern EventGroupHandle_t e;

void wifi_init(void);



