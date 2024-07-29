#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "wifi.h"

EventGroupHandle_t wifi_event;            //WIFI eevent handler
EventGroupHandle_t e;                    //event for MQTT

const char *TAG = "wifi";
int retry = 0;
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry < 10) {
            esp_wifi_connect();
            retry++;
            ESP_LOGI(TAG, "Retrying to connect to the AP");
        } else {
            xEventGroupSetBits(wifi_event, FAIL);                            //Fail bit is set when esp32 reaches max 10 tries
        }
        ESP_LOGI(TAG,"Connection to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry = 0;
        xEventGroupSetBits(wifi_event, CONNECT);                              //connect  bit and C bit is set when esp32 gets IP
        xEventGroupSetBits(e, C);
        
    }
}

void wifi_init(void)
{
    wifi_event = xEventGroupCreate();                         //creating the wifi event group

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();                     //

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();     //WIFI configuration is done with default settings
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
                                                        
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));
                                                        
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,                   //define your wifi SSID in wifi.h file 
            .password = PASS,
           
            .threshold.authmode = WIFI_AUTH_OPEN,           //set the wifi security 
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));                          //WIFI in STATION mode is set
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));            
    ESP_ERROR_CHECK(esp_wifi_start());                                          //Function to to start the wifi******

    ESP_LOGI(TAG, "wifi_init_sta finished.");

//Doesnt wait for both the bits to set and doesn't clear the bits on exiting the finction
    EventBits_t bits = xEventGroupWaitBits(wifi_event, CONNECT | FAIL, pdFALSE, pdFALSE, portMAX_DELAY);              
            
    if (bits & CONNECT) {
        ESP_LOGI(TAG, "Connected to ap %s", SSID);
    } else if (bits & FAIL) {
        ESP_LOGI(TAG, "Failed to connect");
    } else {
        ESP_LOGE(TAG, "Wifi failed");                
    }
}
