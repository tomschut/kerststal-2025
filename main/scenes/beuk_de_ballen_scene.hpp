#ifndef BEUK_DE_BALLEN_SCENE_HPP
#define BEUK_DE_BALLEN_SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scene.hpp"
#include <array>
#include <cmath>
#include <esp_log.h>
#include <tuple>

class BeukDeBallenScene : public Scene {
public:
    BeukDeBallenScene(Lights& strip, DFPlayer& player, Motors& motors)
        : Scene(strip, player, motors)
    {
    }

    void play() override
    {
        ESP_LOGI("BeukDeBallenScene", "Playing Beuk De Ballen Scene");
        motors.getTreeMotor().setSpeed(25);
        player.playBeuk();
        motors.getAngelMotor().setSpeed(40);
        // tot 18.5 langzaam
        strip.sparkeMultipleLeds(23 * 1000, 900); // 18.8 seconds of sparking LEDs
        // tot 25.8
        strip.beatDrop(0, 88, 10.5 * 1000); // 12 seconds beat drop effect
        ESP_LOGI("BeukDeBallenScene", "Starting pulsing effects");
        motors.getTreeMotor().setSpeed(50);
        strip.pulsingChaos(6 * 1000); // 5 seconds of pulsing chaos
        ESP_LOGI("BeukDeBallenScene", "Pulsing beat");
        motors.getTreeMotor().setSpeed(100);
        strip.pulsingBeatInSections(8 * 1000, 3); // 8 seconds of pulsing beat in sections
        strip.turnOff();
        strip.pulsingBeatInSections(13 * 1000, 6); // 13 seconds of pulsing beat in sections

        stop();
    }
};

#endif // BEUK_DE_BALLEN_SCENE_HPP
