#ifndef _DHTTASK_H_
#define _DHTTASK_H_


#include <stdio.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include <dht.h>
#include "httpProtocol.h"

#if defined(CONFIG_DHT_TYPE_DHT11)
#define DHT_SENSOR_TYPE DHT_TYPE_DHT11
#endif
#if defined(CONFIG_DHT_TYPE_AM2301)
#define DHT_SENSOR_TYPE DHT_TYPE_AM2301
#endif
#if defined(CONFIG_DHT_TYPE_SI7021)
#define DHT_SENSOR_TYPE DHT_TYPE_SI7021
#endif

#define DHT_SENSOR_PIN CONFIG_DHT_GPIO_PIN

void dht_task(void *pvParameters);

#endif // !_DHTTASK_H_