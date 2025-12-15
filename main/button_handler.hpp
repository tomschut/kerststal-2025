#ifndef BUTTON_HANDLER_HPP
#define BUTTON_HANDLER_HPP
#include "actuators/motors.hpp" // <-- Add this include
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scenes/scene_handler.hpp"
#include <stdio.h>
#include <tuple>
#include <vector>

class ButtonHandler {
public:
    ButtonHandler(
        Lights& strip, Motors& motors, const gpio_num_t* pins, const gpio_num_t* leds, SceneHandler& sceneHandler)
        : strip_(strip)
        , motors_(motors) // <-- Store reference to motors
        , pins_(pins)
        , leds_(leds)
        , sceneHandler_(sceneHandler)
        , numButtons(sceneHandler.nScenes())
        , buttonTaskHandle_(nullptr)
        , blinkTaskHandle_(nullptr)
        , ambientGlowTaskHandle_(nullptr)
        , keepMotorsStoppedTaskHandle_(nullptr)
    {
    }

    void start()
    {
        xTaskCreate(&ButtonHandler::buttonTaskEntry, "button_task", 4096, this, 5, &buttonTaskHandle_);
        xTaskCreate(&ButtonHandler::blinkTaskEntry, "blink_task", 2048, this, 5, &blinkTaskHandle_);
        xTaskCreate(&ButtonHandler::ambientGlowTaskEntry, "ambient_glow_task", 4096, this, 5, &ambientGlowTaskHandle_);
        xTaskCreate(&ButtonHandler::keepMotorsStoppedTaskEntry, "keep_motors_stopped_task", 2048, this, 5,
            &keepMotorsStoppedTaskHandle_);
    }

    void pauseBlinking()
    {
        if (blinkTaskHandle_)
            vTaskSuspend(blinkTaskHandle_);
    }
    void resumeBlinking()
    {
        if (blinkTaskHandle_)
            vTaskResume(blinkTaskHandle_);
    }
    void pauseAmbientGlow()
    {
        if (ambientGlowTaskHandle_)
            vTaskSuspend(ambientGlowTaskHandle_);
    }
    void resumeAmbientGlow()
    {
        if (ambientGlowTaskHandle_)
            vTaskResume(ambientGlowTaskHandle_);
    }

private:
    Lights& strip_;
    Motors& motors_; // <-- Reference to motors
    const gpio_num_t* pins_;
    const gpio_num_t* leds_;
    SceneHandler& sceneHandler_;
    int numButtons;
    TaskHandle_t buttonTaskHandle_;
    TaskHandle_t blinkTaskHandle_;
    TaskHandle_t ambientGlowTaskHandle_;
    TaskHandle_t keepMotorsStoppedTaskHandle_;

    static void buttonTaskEntry(void* param) { static_cast<ButtonHandler*>(param)->buttonTask(); }
    static void blinkTaskEntry(void* param) { static_cast<ButtonHandler*>(param)->blinkTask(); }
    static void ambientGlowTaskEntry(void* param) { static_cast<ButtonHandler*>(param)->ambientGlowTask(); }
    static void keepMotorsStoppedTaskEntry(void* param) { static_cast<ButtonHandler*>(param)->keepMotorsStoppedTask(); }

    void blinkTask()
    {
        while (true) {
            // Turn all LEDs ON
            for (int i = 0; i < numButtons; ++i) {
                gpio_set_level(leds_[i], 1);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            // Turn all LEDs OFF
            for (int i = 0; i < numButtons; ++i) {
                gpio_set_level(leds_[i], 0);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

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
            if (sceneHandler_.isScenePlaying()) {
                vTaskDelay(pdMS_TO_TICKS(100)); // Skip checking while scene is playing
                pauseBlinking();
                pauseAmbientGlow();
                vTaskSuspend(keepMotorsStoppedTaskHandle_); // <-- Pause keepMotorsStoppedTask
                continue;
            }

            vTaskResume(keepMotorsStoppedTaskHandle_); // <-- Resume after scene
            resumeAmbientGlow();
            resumeBlinking();
            
            for (int i = 0; i < numButtons; ++i) {
                int level = gpio_get_level(pins_[i]);
                if (level == 0) { // button pressed
                    printf("Button %d pressed!\n", i + 1);

                    gpio_set_level(leds_[i], 1);
                    for (int j = 0; j < numButtons; ++j) {
                        if (j != i)
                            gpio_set_level(leds_[j], 0);
                    }
                    strip_.turnOff();
                    sceneHandler_.playScene(i);

                    vTaskDelay(pdMS_TO_TICKS(500)); // debounce
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    void ambientGlowTask()
    {
        while (true) {
            strip_.ambientGlow();
            vTaskDelay(pdMS_TO_TICKS(50)); // Adjust for smoothness
        }
    }

    void keepMotorsStoppedTask()
    {
        while (true) {
            motors_.stopAll(); // Ensure motors are stopped
            vTaskDelay(pdMS_TO_TICKS(100)); // Adjust as necessary
        }
    }
};
#endif // BUTTON_HANDLER_HPP
