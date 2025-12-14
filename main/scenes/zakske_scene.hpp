#ifndef ZAKSKE_SCENE_HPP
#define ZAKSKE_SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scene.hpp"
#include <array>
#include <cmath>
#include <esp_log.h>
#include <tuple>

class ZakskeScene : public Scene {
public:
    ZakskeScene(Lights& strip, DFPlayer& player, Motors& motors)
        : Scene(strip, player, motors)
    {
    }

    void play() override
    {
        ESP_LOGI("ZakskeScene", "Playing Zakske Scene");
        player.playTrack(3);
        wait(2 * 1000); // wait for intro

        // strip.incrementalRainbow(20, 500);
        motors.getMotor(0).setSpeed(100);
        // ----------------------------
        // 16 t/m 22 — LED 21 t/m 35
        // ----------------------------
        strip.setMultipleLeds(21, 35, makeColor(255, 36, 15), 64);
        wait(7000);

        // ----------------------------
        // 23 t/m 25 — LED 52 t/m 62
        // ----------------------------
        // strip.setMultipleLeds(52, 62, makeColor(251, 255, 15), 255);
        ESP_LOGI("ZakskeScene", "Bliksem");
        strip.lightning(50, 80, 5, 100, 200);
        wait(2000);

        // ----------------------------
        // 26 t/m 27 — LED 45 t/m 49
        // ----------------------------
        ESP_LOGI("ZakskeScene", "Stal");
        strip.setMultipleLeds(45, 49, makeColor(255, 15, 243), 128);
        strip.setMultipleLeds(24, 26, makeColor(255, 36, 15), 128);
        wait(2000);

        // ----------------------------
        // 27 t/m 29 — losse leds
        // ----------------------------
        ESP_LOGI("ZakskeScene", "Krib os ezel");
        strip.setLed(24, makeColor(119, 15, 255), 128);
        strip.setLed(25, makeColor(255, 15, 243), 128);
        strip.setLed(26, makeColor(255, 36, 15), 128);
        strip.setLed(27, makeColor(255, 140, 15), 128);
        strip.setLed(28, makeColor(251, 255, 15), 128);
        strip.setLed(29, makeColor(68, 255, 15), 128);
        wait(3000);

        // ----------------------------
        // 31 t/m 38 — reeks losse leds
        // ----------------------------
        ESP_LOGI("ZakskeScene", "ster");
        strip.setLed(51, makeColor(251, 255, 15), 255);
        strip.setLed(55, makeColor(251, 255, 15), 255);
        strip.setLed(58, makeColor(251, 255, 15), 255);
        strip.setLed(61, makeColor(251, 255, 15), 255);
        strip.setLed(63, makeColor(251, 255, 15), 255);
        strip.setLed(65, makeColor(251, 255, 15), 255);
        wait(5000);

        // extra blok 35 t/m 38
        ESP_LOGI("ZakskeScene", "fonkelen");
        strip.setMultipleLeds(61, 65, makeColor(251, 255, 15), 255);
        strip.setMultipleLeds(51, 58, makeColor(251, 255, 15), 255);
        strip.turnOff();
        wait(100);
        strip.setMultipleLeds(61, 65, makeColor(251, 255, 15), 255);
        strip.setMultipleLeds(51, 58, makeColor(251, 255, 15), 255);
        strip.turnOff();
        wait(100);
        strip.setMultipleLeds(61, 65, makeColor(251, 255, 15), 255);
        strip.setMultipleLeds(51, 58, makeColor(251, 255, 15), 255);
        strip.turnOff();
        wait(100);
        strip.setMultipleLeds(61, 65, makeColor(251, 255, 15), 255);
        strip.setMultipleLeds(51, 58, makeColor(251, 255, 15), 255);
        wait(3500);

        // --------
        // 39 — UIT
        // --------
        strip.turnOff();
        wait(100);

        // ----------------------------
        // 40 t/m 42 — om-en-om
        // ----------------------------
        for (int i = 22; i <= 36; i++) {
            if ((i % 2) == 0)
                strip.setLed(i, makeColor(68, 255, 15), 128);
            else
                strip.setLed(i, makeColor(15, 254, 255), 128);
        }
        wait(3000);

        strip.turnOff();
        wait(100);
        // ----------------------------
        // 43
        // ----------------------------
        strip.setLed(30, makeColor(255, 140, 15), 128);
        strip.setLed(32, makeColor(255, 140, 15), 128);
        strip.setLed(34, makeColor(255, 140, 15), 128);
        strip.setLed(29, makeColor(251, 255, 15), 128);
        strip.setLed(31, makeColor(251, 255, 15), 128);
        strip.setLed(33, makeColor(251, 255, 15), 128);
        wait(1000);

        // ----------------------------
        // 44
        // ----------------------------
        strip.setLed(34, makeColor(68, 255, 15), 128);
        strip.setLed(33, makeColor(15, 254, 255), 128);
        wait(2000);

        // ----------------------------
        // 45 t/m 51 — 21 t/m 40 met kleurloop
        ESP_LOGI("ZakskeScene", "Jesuke");
        // ----------------------------
        std::tuple<int, int, int> kleuren[] = {
            makeColor(255, 36, 15),
            makeColor(255, 140, 15),
            makeColor(251, 255, 15),
            makeColor(68, 255, 15),
            makeColor(15, 254, 255),
            makeColor(15, 66, 255),
            makeColor(119, 15, 255),
        };

        for (int m = 0; m < 10; m++) {
            for (int k = 0; k < 7; k++) {
                strip.setMultipleLeds(21, 40, kleuren[k], 255);
                wait(100);
            }
        }

        // ----------------------------
        // 52 t/m 53 — LED 24 t/m 26
        // ----------------------------
        strip.setMultipleLeds(24, 26, makeColor(255, 36, 15), 128);
        wait(2000);
        stop();
    }
};

#endif // ZAKSKE_SCENE_HPP
