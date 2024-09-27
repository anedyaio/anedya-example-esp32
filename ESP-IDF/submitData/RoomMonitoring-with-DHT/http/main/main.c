#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "rtParameter.h"
#include "wifi.h"
#include "dhtTask.h"


void init(void)
{
    ConnectionEvents = xEventGroupCreate();

    xEventGroupSetBits(ConnectionEvents, WIFI_FAIL);
    xEventGroupSetBits(ConnectionEvents, CONNECTION_FAIL);

    xTaskCreate(&wifi_task, "WIFI", 4096, NULL, 1, NULL);
    ESP_LOGI("MAIN","Task will be start will in next 5 seconds");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    xTaskCreate(&dht_task, "send dht data", 4096, NULL, 1, NULL);
}
void app_main(void)
{
    ESP_LOGI("Init", "Restarting device...");
    init();
}