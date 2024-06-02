#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "wifi_config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "dht11.h"
static char* TAG1 = "MQTT";

#define DHT (GPIO_NUM_4)
uint8_t QoS = 1;

void mqtt_event_handler(void* event_handler_arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data);

esp_mqtt_client_handle_t client;

int mqtt_send(const char *topic,const char *payload);
static void dht11_send_message(void *param);

void app_main(void)
{
	// WiFi Initial-----------------
	nvs_init();
	wifi_init_sta();
	//------------------------------
	esp_mqtt_client_config_t esp_mqtt_client_config = {
		.uri = "mqtt://test.mosquitto.org:1883",
	};
	client = esp_mqtt_client_init(&esp_mqtt_client_config);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);

	xTaskCreate(dht11_send_message, "dht11_send_message", 1024*2, NULL, 5, NULL);
}

void mqtt_event_handler(void* event_handler_arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data){
	esp_mqtt_event_handle_t event = event_data;

	switch((esp_mqtt_event_id_t)event_id){
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG1,"MQTT_EVENT_CONNECT");
		esp_mqtt_client_subscribe(client,"itc/gee/#", QoS);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG1,"MQTT_EVENT_DISCONNECTED");
		break;
	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG1,"MQTT_EVENT_SUBSCRIBED");
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG1,"MQTT_EVENT_UNSUBSCRIBED");
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG1,"MQTT_EVENT_PUBLISHED");
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG1,"MQTT_EVENT_DATA");
		printf("topic: %.*s\n",event->topic_len,event->topic);
		printf("message: %.*s\n",event->data_len,event->data);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG1,"Error %s",strerror(event->error_handle->esp_transport_sock_errno));
		break;
	default:
		break;
	}
}
int mqtt_send(const char *topic, const char *payload){
	return esp_mqtt_client_publish(client, topic, payload, strlen(payload), QoS, 0);
}

static void dht11_send_message(void *param){
	char temp[50];
	char humi[50];
	DHT11_init(GPIO_NUM_4);
	for(;;){
        sprintf(temp,"%d \n", DHT11_read().temperature);
        sprintf(humi,"%d\n", DHT11_read().humidity);
        mqtt_send("itc/gee/dht/temp", temp);
        mqtt_send("itc/gee/dht/humi", humi);
		vTaskDelay(5000/portTICK_PERIOD_MS);
	}
}

