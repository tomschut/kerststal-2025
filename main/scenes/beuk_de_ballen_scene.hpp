#ifndef BEUK_DE_BALLEN_SCENE_HPP
#define BEUK_DE_BALLEN_SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "scene.hpp"
#include <array>
#include <cmath>
#include <esp_log.h>
#include <tuple>

class BeukDeBallenScene : public Scene {
public:
    BeukDeBallenScene(Lights& strip, DFPlayer& player, Motors& motors)
        : Scene(strip, player, motors)
        , motor_timer_(nullptr)
    {
    }

    void play() override
    {
        ESP_LOGI("BeukDeBallenScene", "Playing Beuk De Ballen Scene");
        player.setVolume(20);
        player.playBeuk();

        // Start a one-shot timer for 2 seconds to turn on the motors
        motor_timer_ = xTimerCreate(
            "motor_timer", pdMS_TO_TICKS(10 * 1000), pdFALSE, this, &BeukDeBallenScene::motorTimerCallbackStatic);
        xTimerStart(motor_timer_, 0);

        strip.sparkeMultipleLeds(22.5 * 1000, 900); // 23 seconds of sparking LEDs

        // The rest of your scene logic continues as before (non-blocking for the motors)
        strip.beatDrop(0, 88, 10.5 * 1000); // 12 seconds beat drop effect
        ESP_LOGI("BeukDeBallenScene", "Starting pulsing effects");
        motors.getAngelMotor().setSpeed(25);
        motors.getTreeMotor().setSpeed(50);
        strip.pulsingChaos(6 * 1000); // 5 seconds of pulsing chaos
        ESP_LOGI("BeukDeBallenScene", "Pulsing beat");
        motors.getAngelMotor().setSpeed(100);
        motors.getTreeMotor().setSpeed(100);
        strip.pulsingBeatInSections(8 * 1000, 3); // 8 seconds of pulsing beat in sections
        strip.turnOff();
        strip.pulsingBeatInSections(12.5 * 1000, 6); // 12.5 seconds of pulsing beat in sections

        stop();
    }

private:
    TimerHandle_t motor_timer_;

    static void motorTimerCallbackStatic(TimerHandle_t xTimer)
    {
        auto* self = static_cast<BeukDeBallenScene*>(pvTimerGetTimerID(xTimer));
        self->motorTimerCallback();
    }

    void motorTimerCallback()
    {
        motors.getTreeMotor().setSpeed(25);
        motors.getAngelMotor().setSpeed(10);
        ESP_LOGI("BeukDeBallenScene", "Motors turned on after 2 seconds");
    }
};

#endif // BEUK_DE_BALLEN_SCENE_HPP
