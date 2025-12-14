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
#include <chrono> // for timing
#include <utility> // for std::pair
#include <vector>

// Button GPIO pins
constexpr gpio_num_t buttonPins[] = { GPIO_NUM_19, GPIO_NUM_4, GPIO_NUM_21 };
constexpr gpio_num_t ledPins[] = { GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_22 };
constexpr std::array<gpio_num_t, 4> motorPins = { GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_33, GPIO_NUM_23 };

constexpr int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

// Main function
extern "C" void app_main()
{
    esp_log_level_set("*", ESP_LOG_INFO); // Set global logging level to INFO
    esp_log_level_set("LEDStrip", ESP_LOG_DEBUG); // Set specific logging level for LEDStrip class

    Motors motors(motorPins);

    Lights strip = Lights(90, GPIO_NUM_27); // Example: 90 LEDs on GPIO 14
    DFPlayer player;
    player.begin(); // Now includes reset automatically
    wait(1000); // Wait for initialization
    player.setVolume(20);

    ZakskeScene scene1(strip, player, motors);
    BeukDeBallenScene scene2(strip, player, motors);
    HerdertjesScene scene3(strip, player, motors);
    // scene3.play(); // Test play

    std::vector<Scene*> scenes = { &scene1, &scene2, &scene3 };

    ButtonHandler buttons(strip, motors, buttonPins, ledPins, scenes);
    buttons.start();

    ESP_LOGI("Main", "Ready to go");

    // Main thread can do other things
    while (true) {
        // maybe update other logic or monitor sensors
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
