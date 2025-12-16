idf_component_register(
    SRCS "wifi_connect.cpp" "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server esp_wifi
)

include(${CMAKE_SOURCE_DIR}/main/config.cmake)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MQTT_BROKER_IP="${MQTT_BROKER_IP}"
    MQTT_USERNAME="${MQTT_USERNAME}"
    MQTT_PASSWORD="${MQTT_PASSWORD}"
    WIFI_SSID="${WIFI_SSID}"
    WIFI_PASS="${WIFI_PASS}"
)
