#ifndef SCENE_HANDLER_HPP
#define SCENE_HANDLER_HPP
#include "actuators/lights.hpp"
#include "actuators/motors.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "scene.hpp"
#include "web/mqtt_client.hpp"
#include <string>
#include <vector>

class SceneHandler {
public:
    SceneHandler(std::vector<Scene*>* scenes, Lights& strip, Motors& motors, const gpio_num_t* leds, int nButtons,
        MqttClient* mqttClient = nullptr)
        : scenes_(scenes)
        , strip_(strip)
        , motors_(motors)
        , leds_(leds)
        , mqttClient_(mqttClient)
        , numButtons(nButtons)
    {
        nvs_flash_init();
        loadPlayCounts();
    }

    void start()
    {
        xTaskCreate(&SceneHandler::ambientGlowTaskEntry, "ambient_glow_task", 4096, this, 5, &ambientGlowTaskHandle_);
        xTaskCreate(&SceneHandler::blinkTaskEntry, "blink_task", 2048, this, 5, &blinkTaskHandle_);
        xTaskCreate(&SceneHandler::keepMotorsStoppedTaskEntry, "keep_motors_stopped_task", 2048, this, 5,
            &keepMotorsStoppedTaskHandle_);
        xTaskCreate(&SceneHandler::backgroundTaskManagerEntry, "background_task_manager", 2048, this, 5, nullptr);
        // GPIO setup (same as before)
        for (int i = 0; i < numButtons; ++i) {
            gpio_reset_pin(leds_[i]);
            gpio_set_direction(leds_[i], GPIO_MODE_OUTPUT);
        }
    }

    void playScene(size_t index)
    {
        if (index < scenes_->size() && !isScenePlaying()) {

            currentScene = index;
            xTaskCreate(&SceneHandler::sceneTaskEntry, "scene_task", 4096, this, 5, &sceneTaskHandle_);
        }
    }


    void stopScene()
    {
        if (sceneTaskHandle_ != nullptr) {
            // Call stop() on the current scene before killing the task
            if (currentScene >= 0 && currentScene < scenes_->size()) {
                (*scenes_)[currentScene]->stop();
            }
            vTaskDelete(sceneTaskHandle_);
            sceneTaskHandle_ = nullptr;
            currentScene = -1;
        }
    }

    int nScenes() const { return scenes_->size(); }
    bool isScenePlaying() const { return currentScene != -1; }

    // --- New: per-scene play count access ---
    int getPlayCount(size_t index) const
    {
        if (index < playCounts_.size())
            return playCounts_[index];
        return 0;
    }

    void resetPlayCounts()
    {
        for (auto& c : playCounts_)
            c = 0;
        savePlayCounts();
    }

    int getCurrentScene() const { return currentScene; }

private:
    std::vector<Scene*>* scenes_;
    Lights& strip_;
    Motors& motors_;
    const gpio_num_t* leds_;
    MqttClient* mqttClient_;
    int numButtons;
    int currentScene { -1 };
    TaskHandle_t sceneTaskHandle_ = nullptr;
    TaskHandle_t ambientGlowTaskHandle_ = nullptr;
    TaskHandle_t blinkTaskHandle_ = nullptr;
    TaskHandle_t keepMotorsStoppedTaskHandle_ = nullptr;
    std::vector<int> playCounts_;

    static void sceneTaskEntry(void* param) { static_cast<SceneHandler*>(param)->sceneTask(); }

    void sceneTask()
    {
        if (currentScene >= 0 && currentScene < scenes_->size()) {
            if (mqttClient_) {
                mqttClient_->publish("nativity/scenePlay", std::to_string(currentScene).c_str());
            }
            (*scenes_)[currentScene]->play();
            playCounts_[currentScene]++;
            // savePlayCounts(); //todo turn on voor echt
        }
        currentScene = -1;
        sceneTaskHandle_ = nullptr;
        vTaskDelete(nullptr);
    }

    // --- NVS persistence helpers ---
    void savePlayCounts()
    {
        nvs_handle_t nvs_handle;
        if (nvs_open("scene_ns", NVS_READWRITE, &nvs_handle) == ESP_OK) {
            for (size_t i = 0; i < playCounts_.size(); ++i) {
                std::string key = "scene" + std::to_string(i);
                nvs_set_i32(nvs_handle, key.c_str(), playCounts_[i]);
            }
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
        }
    }

    void loadPlayCounts()
    {
        playCounts_.resize(scenes_ ? scenes_->size() : 0, 0);
        nvs_handle_t nvs_handle;
        if (nvs_open("scene_ns", NVS_READONLY, &nvs_handle) == ESP_OK) {
            for (size_t i = 0; i < playCounts_.size(); ++i) {
                std::string key = "scene" + std::to_string(i);
                int32_t value = 0;
                nvs_get_i32(nvs_handle, key.c_str(), &value);
                playCounts_[i] = value;
            }
            nvs_close(nvs_handle);
        }
    }

    static void ambientGlowTaskEntry(void* param) { static_cast<SceneHandler*>(param)->ambientGlowTask(); }
    static void blinkTaskEntry(void* param) { static_cast<SceneHandler*>(param)->blinkTask(); }
    static void keepMotorsStoppedTaskEntry(void* param) { static_cast<SceneHandler*>(param)->keepMotorsStoppedTask(); }
    static void backgroundTaskManagerEntry(void* param) { static_cast<SceneHandler*>(param)->backgroundTaskManager(); }

    void ambientGlowTask()
    {
        while (true) {
            if (!isScenePlaying()) {
                strip_.ambientGlow();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

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

    void keepMotorsStoppedTask()
    {
        while (true) {
            if (!isScenePlaying()) {
                motors_.stopAll();
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    void backgroundTaskManager()
    {
        bool wasPlaying = false;
        while (true) {
            bool nowPlaying = isScenePlaying();
            if (nowPlaying && !wasPlaying) {

                int i = getCurrentScene();
                gpio_set_level(leds_[i], 1);
                for (int j = 0; j < numButtons; ++j) {
                    if (j != i)
                        gpio_set_level(leds_[j], 0);
                }
                // Scene just started: suspend background tasks
                if (ambientGlowTaskHandle_)
                    vTaskSuspend(ambientGlowTaskHandle_);
                if (blinkTaskHandle_)
                    vTaskSuspend(blinkTaskHandle_);
                if (keepMotorsStoppedTaskHandle_)
                    vTaskSuspend(keepMotorsStoppedTaskHandle_);
            } else if (!nowPlaying && wasPlaying) {
                // Scene just ended: resume background tasks
                if (ambientGlowTaskHandle_)
                    vTaskResume(ambientGlowTaskHandle_);
                if (blinkTaskHandle_)
                    vTaskResume(blinkTaskHandle_);
                if (keepMotorsStoppedTaskHandle_)
                    vTaskResume(keepMotorsStoppedTaskHandle_);
            }
            wasPlaying = nowPlaying;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
};

#endif // SCENE_HANDLER_HPP
