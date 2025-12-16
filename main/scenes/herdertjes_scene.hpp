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
        player.setVolume(30);
        ESP_LOGI("HerdertjesScene", "Playing ruimtelijke Herdertjes Scene");
        player.playShepherd();

        // Wait 3 seconds before starting
        wait(3000);

        // Kleuren
        auto nightBlue = std::make_tuple(25, 40, 90);
        auto deepGreen = std::make_tuple(30, 90, 50);
        auto warmGold = std::make_tuple(240, 220, 140);
        auto softPurple = std::make_tuple(120, 90, 160);
        auto softWhite = std::make_tuple(255, 255, 255);

        // =======================================
        // FASE 1 – Nacht valt (alles komt op)
        // =======================================
        motors.getTreeMotor().setSpeed(15); // Use a higher speed to avoid stutter
        motors.getAngelMotor().stop();

        // Hemel
        for (int b = 0; b <= 120; b += 3) {
            strip.setMultipleLeds(0, 29, nightBlue, b);
            wait(80);
        }

        // Veld
        for (int b = 0; b <= 100; b += 3) {
            strip.setMultipleLeds(30, 69, deepGreen, b);
            wait(80);
        }

        // Herders (zacht goud)
        for (int b = 0; b <= 140; b += 4) {
            strip.setMultipleLeds(70, 83, warmGold, b);
            wait(90);
        }

        // =======================================
        // FASE 2 – Rustige ademhaling
        // =======================================
        for (int cycle = 0; cycle < 2; cycle++) {
            for (int b = 110; b <= 160; b += 3) {
                strip.setMultipleLeds(70, 83, warmGold, b);
                strip.setMultipleLeds(30, 69, deepGreen, b - 30);
                wait(120);
            }
            for (int b = 160; b >= 110; b -= 3) {
                strip.setMultipleLeds(70, 83, warmGold, b);
                strip.setMultipleLeds(30, 69, deepGreen, b - 30);
                wait(120);
            }
        }

        wait(2 * 1000);
        // =======================================
        // FASE 3 – Aankondiging (paars licht)
        // =======================================
        motors.getAngelMotor().setSpeed(-10);

        for (int b = 0; b <= 160; b += 4) {
            strip.setMultipleLeds(0, 29, softPurple, b);
            strip.setMultipleLeds(70, 83, warmGold, 150);
            strip.setMultipleLeds(84, 88, softWhite, b);
            wait(90);
        }

        // =======================================
        // FASE 4 – Engelen zingen
        // =======================================
        motors.getTreeMotor().setSpeed(15); // Use a higher speed to avoid stutter
        motors.getAngelMotor().setSpeed(-25);

        for (int pulse = 0; pulse < 3; pulse++) {
            for (int b = 160; b <= 240; b += 4) {
                strip.setMultipleLeds(70, 83, warmGold, b);
                strip.setMultipleLeds(84, 88, softWhite, b);
                strip.setMultipleLeds(0, 29, softPurple, b - 40);
                wait(80);
            }
            for (int b = 240; b >= 160; b -= 4) {
                strip.setMultipleLeds(70, 83, warmGold, b);
                strip.setMultipleLeds(84, 88, softWhite, b);
                strip.setMultipleLeds(0, 29, softPurple, b - 40);
                wait(80);
            }
        }

        // =======================================
        // FASE 5 – Terug naar stilte
        // =======================================
        motors.getAngelMotor().setSpeed(-5);
        motors.getTreeMotor().setSpeed(15); // Use a higher speed to avoid stutter

        for (int b = 140; b >= 0; b -= 4) {
            strip.setMultipleLeds(70, 83, warmGold, b);
            strip.setMultipleLeds(84, 88, softWhite, b);
            strip.setMultipleLeds(0, 69, nightBlue, b / 2);
            wait(100);
        }

        stop();
    }
};

#endif // HERDERTJES_SCENE_HPP
