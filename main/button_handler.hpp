#ifndef BUTTON_HANDLER_HPP
#define BUTTON_HANDLER_HPP

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scenes/scene_handler.hpp"
#include <stdio.h>
#include <tuple>
#include <vector>

class ButtonHandler {
public:
    ButtonHandler(const gpio_num_t* pins, SceneHandler& sceneHandler)
        : pins_(pins)
        , sceneHandler_(sceneHandler)
        , numButtons(sceneHandler.nScenes())
        , buttonTaskHandle_(nullptr)
    {
    }

    void start() { xTaskCreate(&ButtonHandler::buttonTaskEntry, "button_task", 4096, this, 5, &buttonTaskHandle_); }

private:
    const gpio_num_t* pins_;
    SceneHandler& sceneHandler_;
    int numButtons;
    TaskHandle_t buttonTaskHandle_;

    static void buttonTaskEntry(void* param) { static_cast<ButtonHandler*>(param)->buttonTask(); }

    void buttonTask()
    {
        for (int i = 0; i < numButtons; ++i) {
            gpio_config_t io_conf {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pin_bit_mask = (1ULL << pins_[i]);
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            gpio_config(&io_conf);
        }

        while (true) {
            // Read all button levels
            std::vector<int> levels(numButtons);
            for (int i = 0; i < numButtons; ++i) {
                levels[i] = gpio_get_level(pins_[i]);
            }

            // If button 0 and 2 are pressed at the same time, stop the scene
            if (levels[0] == 0 && levels[2] == 0) {
                ESP_LOGI("ButtonHandler", "Buttons 1 and 3 pressed together: stopping scene");
                sceneHandler_.stopScene();
                vTaskDelay(pdMS_TO_TICKS(500)); // debounce
            } else {
                // Normal button handling
                for (int i = 0; i < numButtons; ++i) {
                    if (levels[i] == 0) { // button pressed
                        ESP_LOGI("ButtonHandler", "Button %d pressed", i + 1);
                        vTaskDelay(pdMS_TO_TICKS(100)); // debounce
                        sceneHandler_.playScene(i);
                        vTaskDelay(pdMS_TO_TICKS(500)); // debounce
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
};
#endif // BUTTON_HANDLER_HPP
