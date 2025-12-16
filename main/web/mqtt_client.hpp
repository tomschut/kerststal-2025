#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

#include "esp_log.h"
#include "mqtt_client.h"

class MqttClient {
public:
    explicit MqttClient()
        : client_(nullptr)
    {
    }

    void start()
    {
        esp_mqtt_client_config_t mqtt_cfg = {};
        mqtt_cfg.broker.address.uri = MQTT_BROKER_IP;
        mqtt_cfg.credentials.username = MQTT_USERNAME;
        mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
        mqtt_cfg.session.disable_clean_session = false;
        mqtt_cfg.session.keepalive = 60;

        client_ = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, &MqttClient::event_handler_static, this);
        esp_mqtt_client_start(client_);
    }

    void publish(const char* topic, const char* payload)
    {
        if (client_) {
            esp_mqtt_client_publish(client_, topic, payload, 0, 1, 0);
        }
    }

private:
    esp_mqtt_client_handle_t client_;

    static void event_handler_static(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
    {
        MqttClient* self = static_cast<MqttClient*>(handler_args);
        self->event_handler(base, event_id, event_data);
    }

    void event_handler(esp_event_base_t base, int32_t event_id, void* event_data)
    {
        if (event_id == MQTT_EVENT_CONNECTED) {
            ESP_LOGI("mqtt", "MQTT connected");
        }
    }
};

#endif // MQTT_CLIENT_HPP
