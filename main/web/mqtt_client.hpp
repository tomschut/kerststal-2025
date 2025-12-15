#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP
#include "esp_log.h"
#include "mqtt_client.h"

static const char* TAG = "mqtt";
static esp_mqtt_client_handle_t client = nullptr;

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "MQTT connected");
    }
}

void mqtt_client_start()
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_IP;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void publish_mqtt_message(const char* topic, const char* payload)
{
    if (client) {
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0); // Non-blocking
    }
}

#endif // MQTT_CLIENT_HPP
