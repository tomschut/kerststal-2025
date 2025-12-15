#ifndef SCENE_HANDLER_HPP
#define SCENE_HANDLER_HPP
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "scene.hpp"
#include <string>
#include <vector>

class SceneHandler {
public:
    SceneHandler(std::vector<Scene*>* scenes)
        : scenes_(scenes)
    {
        nvs_flash_init();
        loadPlayCounts();
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
    int currentScene { -1 };
    TaskHandle_t sceneTaskHandle_ = nullptr;
    std::vector<int> playCounts_;

    static void sceneTaskEntry(void* param) { static_cast<SceneHandler*>(param)->sceneTask(); }

    void sceneTask()
    {
        if (currentScene >= 0 && currentScene < scenes_->size()) {
            (*scenes_)[currentScene]->play();
            playCounts_[currentScene]++;
            savePlayCounts();
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
};

#endif // SCENE_HANDLER_HPP
