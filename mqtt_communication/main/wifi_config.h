/*
 * wifi_config.h
 *
 *  Created on: Dec 11, 2023
 *      Author: rithy LIM
 */

#ifndef MAIN_WIFI_CONFIG_H_
#define MAIN_WIFI_CONFIG_H_
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "stdio.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID      "AI FARM ROBOTICS FACTORY UNIFI"
#define EXAMPLE_ESP_WIFI_PASS      "@AIFARM2022"
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

static const char *TAG = "wifi station";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//static const char *TAG = "wifi station";

static int s_retry_num = 0;

void nvs_init(void);

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
void wifi_init_sta(void);


#endif /* MAIN_WIFI_CONFIG_H_ */
