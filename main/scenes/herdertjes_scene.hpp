#ifndef HERDERTJES_SCENE_HPP
#define HERDERTJES_SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"
#include "../util.hpp" // for wait function
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scene.hpp"
#include <array>
#include <cmath>
#include <esp_log.h>
#include <tuple>

class HerdertjesScene : public Scene {
public:
    HerdertjesScene(Lights& strip, DFPlayer& player, Motors& motors)
        : Scene(strip, player, motors)
    {
    }

    void play() override
    {
        player.setVolume(25);
        ESP_LOGI("HerdertjesScene", "Playing Herdertjes Scene");
        player.playTrack(1);
        motors.getMotor(2).setSpeed(100);

        // 10 t/m 32 — kleur R251 G255 B15 op LED 70 t/m 83
        strip.setMultipleLeds(70, 83, std::make_tuple(251, 255, 15), 255);
        wait((32 - 10 + 1) * 1000); // of jouw gewenste tijd

        // 33 t/m 44 — kleur R255 G255 B255 op LED 84
        strip.setLed(84, std::make_tuple(255, 255, 255), 255);
        wait((44 - 33 + 1) * 1000);

        // 33 t/m 44 — kleur R255 G255 B255 op LED 85
        strip.setLed(85, std::make_tuple(255, 255, 255), 255);
        wait((44 - 33 + 1) * 1000);

        // 45 t/m 54 — kleur R251 G255 B15 op LED 70 t/m 83
        strip.setMultipleLeds(70, 83, std::make_tuple(251, 255, 15), 255);
        wait((54 - 45 + 1) * 1000);

        stop();
    }
};

#endif // HERDERTJES_SCENE_HPP
