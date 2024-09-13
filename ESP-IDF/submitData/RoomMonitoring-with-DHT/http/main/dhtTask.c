#include "dhtTask.h"

static const char* TAG = "DHT_TASK";
void dht_task(void *pvParameters)
{
    float temperature, humidity;
    while (1)
    {
        if (dht_read_float_data(DHT_SENSOR_TYPE, DHT_SENSOR_PIN, &humidity, &temperature) == ESP_OK)
        {
            ESP_LOGI(TAG,"Humidity: %.1f%% Temp: %.1fC", humidity, temperature);

            // Allocate buffer dynamically using asprintf
            char *payload = NULL;
            int len = asprintf(&payload, "{\"data\":[{\"variable\": \"temperature\",\"value\":%0.2f,\"timestamp\":%d},{\"variable\": \"humidity\",\"value\":%0.2f,\"timestamp\":%d}]}", temperature, 0,humidity,0);
            if (len < 0)
            {
                ESP_LOGI(TAG,"[Temperature] Error allocating buffer\n");
                continue;
            }

            // Allocate memory for req_ctx_t struct dynamically
            req_ctx_t *temperature_ctx_t = malloc(sizeof(req_ctx_t));
            temperature_ctx_t->url = "https://device.ap-in-1.anedya.io/v1/submitData";
            temperature_ctx_t->body = (uint_least8_t *)payload;
            doPost(temperature_ctx_t);

            free(payload);
            free(temperature_ctx_t);
        }
        else
        {
            ESP_LOGI(TAG,"[test-03]Could not read data from sensor");
        }
        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}