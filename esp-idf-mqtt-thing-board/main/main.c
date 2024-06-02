//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_wifi.h"
//#include "esp_system.h"
//#include "esp_event.h"
//#include "esp_event_loop.h"
//#include "nvs_flash.h"
//#include "driver/gpio.h"
//#include <stdio.h>
//#include "wifi_config.h"
//#include "esp_log.h"
//#include "mqtt_client.h"
//#include "dht11.h"
//#include ""
//static char* TAG1 = "MQTT";
//
//
//
//#define DHT (GPIO_NUM_4)
//
//uint8_t QoS = 1;
//
//void mqtt_event_handler(void* event_handler_arg,
//        esp_event_base_t event_base,
//        int32_t event_id,
//        void* event_data);
//
//esp_mqtt_client_handle_t client;
//
//int mqtt_send(const char *topic,const char *payload);
//static void dht11_send_message(void *param);
//
//void app_main(void)
//{
//	// WiFi Initial-----------------
//	nvs_init();
//	wifi_init_sta();
//	//------------------------------
//	esp_mqtt_client_config_t esp_mqtt_client_config = {
//		.uri = "mqtt://test.mosquitto.org:1883",
//	};
//	client = esp_mqtt_client_init(&esp_mqtt_client_config);
//	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
//	esp_mqtt_client_start(client);
//
//	xTaskCreate(dht11_send_message, "dht11_send_message", 1024*2, NULL, 5, NULL);
//}
//
//void mqtt_event_handler(void* event_handler_arg,
//        esp_event_base_t event_base,
//        int32_t event_id,
//        void* event_data){
//	esp_mqtt_event_handle_t event = event_data;
//
//	switch((esp_mqtt_event_id_t)event_id){
//	case MQTT_EVENT_CONNECTED:
//		ESP_LOGI(TAG1,"MQTT_EVENT_CONNECT");
//		esp_mqtt_client_subscribe(client,"itc/gee/daniel", QoS);
//		esp_mqtt_client_subscribe(client,"itc/gee/lyvong", QoS);
//		break;
//	case MQTT_EVENT_DISCONNECTED:
//		ESP_LOGI(TAG1,"MQTT_EVENT_DISCONNECTED");
//		break;
//	case MQTT_EVENT_SUBSCRIBED:
//		ESP_LOGI(TAG1,"MQTT_EVENT_SUBSCRIBED");
//		break;
//	case MQTT_EVENT_UNSUBSCRIBED:
//		ESP_LOGI(TAG1,"MQTT_EVENT_UNSUBSCRIBED");
//		break;
//	case MQTT_EVENT_PUBLISHED:
//		ESP_LOGI(TAG1,"MQTT_EVENT_PUBLISHED");
//		break;
//	case MQTT_EVENT_DATA:
//		ESP_LOGI(TAG1,"MQTT_EVENT_DATA");
//		printf("topic: %.*s\n",event->topic_len,event->topic);
//		printf("message: %.*s\n",event->data_len,event->data);
//		break;
//	case MQTT_EVENT_ERROR:
//		ESP_LOGI(TAG1,"Error %s",strerror(event->error_handle->esp_transport_sock_errno));
//		break;
//	default:
//		break;
//	}
//}
//int mqtt_send(const char *topic, const char *payload){
//	return esp_mqtt_client_publish(client, topic, payload, strlen(payload), QoS, 0);
//}
//
//static void dht11_send_message(void *param){
//	char temp[50];
//	char humi[50];
//	DHT11_init(GPIO_NUM_4);
//	for(;;){
//        sprintf(temp,"Temperature is %d \n", DHT11_read().temperature);
//        sprintf(humi,"Humidity is %d\n", DHT11_read().humidity);
//        mqtt_send("itc/gee/dht/temp", temp);
//        mqtt_send("itc/gee/dht/humi", humi);
//		vTaskDelay(5000/portTICK_PERIOD_MS);
//	}
//}

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "DHT22.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "ThingBoard.io";
#define  EXAMPLE_ESP_WIFI_SSID "wong"
#define  EXAMPLE_ESP_WIFI_PASS "12345678"

#define topics_sub "v1/devices/me/rpc/request/+"
#define topics_pub "v1/devices/me/telemetry"
#define topics_pub1 "v1/devices/me/attributes"
#define broker "mqtt://tW6oTBcOJpw145v24gwc:NULL@demo.thingsboard.io:1883"

uint32_t MQTT_CONNEECTED = 0;
static void mqtt_app_start(void);

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "Trying to connect with Wi-Fi\n");
        break;

    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Wi-Fi connected\n");
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip: starting MQTT Client\n");
        mqtt_app_start();
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "disconnected: Retrying Wi-Fi\n");
        esp_wifi_connect();
        break;

    default:
        break;
    }
    return ESP_OK;
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
       .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNEECTED=1;

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test1", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNEECTED=0;
        break;
    case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
        }
    }

    esp_mqtt_client_handle_t client = NULL;
    static void mqtt_app_start(void)
    {
        ESP_LOGI(TAG, "STARTING MQTT");
        esp_mqtt_client_config_t mqttConfig = {
            .uri = broker,
        };

        client = esp_mqtt_client_init(&mqttConfig);
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
        esp_mqtt_client_start(client);
    }

    void Publisher_Task(void *params)
    {
      while (true)
      {
        if(MQTT_CONNEECTED)
        {
            // Send data to ThingBoard (replace "temperature" and "22.5" with your actual data)
        	setDHTgpio( 4 );
        	int ret = readDHT();

        			errorHandler(ret);


              char topic[64];
              sprintf(topic, "v1/devices/me/telemetry");
              char payload[64];
              sprintf(payload, "{\"temperature\": %f}", getTemperature());

              esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);

              sprintf(topic, "v1/devices/me/telemetry");
              sprintf(payload, "{\"humidity\": %f}",getHumidity());

              esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);

              vTaskDelay(1000 / portTICK_RATE_MS);
        }
      }
    }


//    void Publisher_Task1(void *parameters)
//       {
//         while (true)
//         {
//           if(MQTT_CONNEECTED)
//           {
//               // Send data to ThingBoard (replace "temperature" and "22.5" with your actual data)
//           	setDHTgpio( 4 );
//           	int ret = readDHT();
//
//           	errorHandler(ret);
//
//
//                 char topic[64];
//                 sprintf(topic, "v1/devices/me/telemetry");
//                 char payload[64];
//                 sprintf(payload, "{\"temperature\": %f}", getTemperature());
//
//                 esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
//
//                 vTaskDelay(2000 / portTICK_RATE_MS);
//           }
//         }
//       }
//
//
//    void Publisher_Task2(void *params)
//           {
//             while (true)
//             {
//               if(MQTT_CONNEECTED)
//               {
//                   // Send data to ThingBoard (replace "temperature" and "22.5" with your actual data)
//               	setDHTgpio( 4 );
//               	int ret = readDHT();
//
//               	errorHandler(ret);
//
//                     char topic[64];
//                     sprintf(topic, "v1/devices/me/telemetry");
//                     char payload[64];
//                     sprintf(payload, "{\"humidity\": %f}",getHumidity());
//
//                     esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
//
//                     vTaskDelay(2000 / portTICK_RATE_MS);
//               }
//             }
//           }
    void app_main(void)
    {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
        wifi_init();
        xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
       // xTaskCreate(Publisher_Task2, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
    }

