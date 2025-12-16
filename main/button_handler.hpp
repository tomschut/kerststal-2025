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
    ButtonHandler(const gpio_num_t* pins, const gpio_num_t* leds, SceneHandler& sceneHandler)
        : pins_(pins)
        , leds_(leds)
        , sceneHandler_(sceneHandler)
        , numButtons(sceneHandler.nScenes())
        , buttonTaskHandle_(nullptr)
    {
    }

    void start() { xTaskCreate(&ButtonHandler::buttonTaskEntry, "button_task", 4096, this, 5, &buttonTaskHandle_); }

private:
    const gpio_num_t* pins_;
    const gpio_num_t* leds_;
    SceneHandler& sceneHandler_;
    int numButtons;
    TaskHandle_t buttonTaskHandle_;

    static void buttonTaskEntry(void* param) { static_cast<ButtonHandler*>(param)->buttonTask(); }

    void buttonTask()
    {
        // GPIO setup (same as before)
        for (int i = 0; i < numButtons; ++i) {
            gpio_reset_pin(leds_[i]);
            gpio_set_direction(leds_[i], GPIO_MODE_OUTPUT);
        }
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
            for (int i = 0; i < numButtons; ++i) {
                int level = gpio_get_level(pins_[i]);
                if (level == 0) { // button pressed
                    ESP_LOGI("ButtonHandler", "Button %d pressed", i + 1);

                    gpio_set_level(leds_[i], 1);
                    for (int j = 0; j < numButtons; ++j) {
                        if (j != i)
                            gpio_set_level(leds_[j], 0);
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); // debounce
                    sceneHandler_.playScene(i);
                    vTaskDelay(pdMS_TO_TICKS(500)); // debounce
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
};
#endif // BUTTON_HANDLER_HPP
