#include "actuators/dfplayer.hpp"
#include "actuators/lights.hpp"
#include "actuators/motors.hpp"
#include "button_handler.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scenes/beuk_de_ballen_scene.hpp"
#include "scenes/herdertjes_scene.hpp"
#include "scenes/zakske_scene.hpp"
#include "util.hpp" // for wait function
#include "web/mqtt_client.hpp"
#include "web/web_server.hpp"
#include "wifi_connect.cpp" // or use a header if you have one
#include <chrono> // for timing
#include <utility> // for std::pair
#include <vector>

// Button GPIO pins
constexpr gpio_num_t buttonPins[] = { GPIO_NUM_19, GPIO_NUM_4, GPIO_NUM_21 };
constexpr gpio_num_t ledPins[] = { GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_22 };
constexpr std::array<gpio_num_t, 4> motorPins = { GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_33, GPIO_NUM_23 };

constexpr int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

// SceneHandler global pointer
SceneHandler* g_sceneHandler = nullptr;

// Main function
extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("DFPlayer", ESP_LOG_WARN);
    esp_log_level_set("ButtonHandler", ESP_LOG_WARN);

    wifi_connect();

    Motors motors(motorPins);
    Lights strip = Lights(89, GPIO_NUM_27);
    DFPlayer player;
    player.begin();

    ZakskeScene scene1(strip, player, motors);
    BeukDeBallenScene scene2(strip, player, motors);
    HerdertjesScene scene3(strip, player, motors);

    std::vector<Scene*> scenes = { &scene1, &scene2, &scene3 };

    MqttClient mqttClient;
    mqttClient.start();

    SceneHandler sceneHandler(&scenes, strip, motors, ledPins, 3, &mqttClient);
    sceneHandler.start();
    ButtonHandler buttons(buttonPins, sceneHandler);
    buttons.start();

    WebServer webServer(&sceneHandler);
    webServer.start();
    ESP_LOGI("Main", "Ready to go");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
