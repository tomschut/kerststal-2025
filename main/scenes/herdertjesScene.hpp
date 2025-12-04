#ifndef HERDERTJES_SCENE_HPP
#define HERDERTJES_SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scene.hpp"
#include <esp_log.h>

class HerdertjesScene : public Scene {
public:
    HerdertjesScene(Lights& strip, DFPlayer& dfplayer)
        : Scene(strip, dfplayer)
    {
    }

    void play()
    {
        // Implementation of the Herdertjes scene playback
        ESP_LOGI("HerdertjesScene", "Playing Herdertjes Scene");
        // begin
        dfplayer.playTrack(2); // Play the Herdertjes audio track
        // wacht 10 seconden
        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 10 seconds
        // licht aan
        strip.setLeds(0, 60, std::make_tuple(255, 255, 0)); // Set first LED to yellow

        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 10 seconds
        strip.turnOff();
        strip.setLeds(60, 90, std::make_tuple(0, 0, 0)); // Turn off LEDs
    };
};

#endif // HERDERTJES_SCENE_HPP
