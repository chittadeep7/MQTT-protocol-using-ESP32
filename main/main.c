#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "mqtt_client.h"

#include "MPU6050.h"
#include "wifi.h"

#define x  (1 << 0)
#define y  (1 << 1)
//#define z  (1 << 3)

static const char *TAG1 = "mqtt";
esp_mqtt_client_handle_t client = NULL;                    //mqtt client handle defined


//can be used to put the esp32 to sleep to reduce power consumption
//ESP32 wakes up after a specified time and starts the WIFI which sets C bit of mqqt event handler
//stop the wifi before calling this function
//deep sleep will restart the esp32
void E_sleep(void)
{
    MPU_sleep(MPU6050_address);
    esp_sleep_enable_timer_wakeup(5000000);                        //5000000 us
    printf("Entering sleep mode\n");

    int64_t t_before_us = esp_timer_get_time();

    esp_light_sleep_start();
    int64_t t_after_us = esp_timer_get_time();                      //programme resumes here after after waking up from light sleep
    
    printf("Returned from deep sleep, slept for time %lld s\n", (t_after_us/1000000 - t_before_us/1000000));
    ESP_ERROR_CHECK(esp_wifi_start());
}


//MQTT handler that runs when mqtt_start function is called
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG1, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG1, "MQTT CONNECTED");                                 
        //xTaskNotify(taskH, M, eSetValueWithOverwrite);
        xEventGroupSetBits(e, x);                                            //x bit is set when mqtt is connected

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG1, "MQTT DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG1, "MQTT SUBSCRIBED");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG1, "MQTT UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG1, "MQTT PUBLISHED");
        //xTaskNotify(taskH, P, eSetValueWithOverwrite);
        xEventGroupSetBits(e, y);                                             //y bit is set when mqtt is connected
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG1, "MQTT EVENT DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    default:
        ESP_LOGI(TAG1, "Other event id:%d", event->event_id);
        break;
    }
}


//Main task that starts the mqtt client when wifi is connected
void mqtt_app(void *arg)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://mqtt.eclipseprojects.io",                //mqtt broker address
    };
     while (1)
     {
        EventBits_t q = xEventGroupWaitBits(e, x | y | C, pdTRUE, pdFALSE, portMAX_DELAY);        //waits for any of the bits to set
        if((q & C) != 0)                                                                          //when esp32 gets IP this runs
        {
            //ESP_ERROR_CHECK(esp_wifi_start());
            client = esp_mqtt_client_init(&mqtt_cfg);
            esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
            esp_mqtt_client_start(client);
        }
        else if((q & x) != 0)
        {                                                                      //when mqtt is connected
            MPU();                                                             // reads all the sensor data
            
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_mqtt_client_subscribe(client, "topic/MPU", 1);                 //subscribe to the tpic you are publishng on so you also 
                                                                               //receive the data
            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "Ax", map->accx);
            cJSON_AddNumberToObject(root, "Ay", map->accy);
            cJSON_AddNumberToObject(root, "Az", map->accz);                    //prints all the data in json format using cJSON
            cJSON_AddNumberToObject(root, "Temperature", map->tem);            //or use spritnf function
            cJSON_AddNumberToObject(root, "Gx", map->gyx);
            cJSON_AddNumberToObject(root, "Gy", map->gyy);
            cJSON_AddNumberToObject(root, "Gz", map->gyz);
            char *info = cJSON_Print(root);
            size_t inf = strlen(info);
            esp_mqtt_client_publish(client, "topic/MPU", info, inf, 1, false);   //publishes the data
            free(info);
            cJSON_Delete(root);
        }
        else if((q & y) != 0)
        {
            esp_mqtt_client_stop(client);
            esp_mqtt_client_destroy(client);                         //when data is published stop the client and enter sleep mode
            esp_wifi_stop();
            //vTaskDelay(2000 / portTICK_PERIOD_MS);
            E_sleep();

        }
  }
}

/*
void task(void *arg)                          //Use this to start the mqtt client if you don't want to use sleep mode 
{
    while(1)                                  //use the z bit to start the mqtt client
    {
        xEventGroupSetBits(e, z);
        vTaskDelay(15000 / portTICK_PERIOD_MS);        //delay before mqtt is connected again and publishes data

    }
}
*/

void app_main(void)
{
    e = xEventGroupCreate();
    
    //initializes the I@C configuration
    i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = 26,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_io_num = 27,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 100000,              //100kHz
      .clk_flags = 0
  };
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  printf("I2C configured\n");

  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
  printf("I2C driver installed\n");
  nvs_flash_init();

  wifi_init();            

  xTaskCreate(mqtt_app, "mqtt", 1024 * 5, NULL, 5, NULL);
  //xTaskCreate(task, "task2", 1024 * 5, NULL, 5, NULL);                 //use this if sleep mode is not required

}
