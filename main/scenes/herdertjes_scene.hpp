#ifndef HERDERTJES_SCENE_HPP
#define HERDERTJES_SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"
#include "../util.hpp" // for wait function
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scene.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <esp_log.h>
#include <tuple>
#include <vector>

class HerdertjesScene : public Scene {
public:
    HerdertjesScene(Lights& strip, DFPlayer& player, Motors& motors)
        : Scene(strip, player, motors)
    {
    }

    void play() override
    {
        player.setVolume(30);
        ESP_LOGI("HerdertjesScene", "Playing levendige Herdertjes Scene");
        player.playShepherd();

        strip.runningOppositeNoNeighbors(8 * 1000, 150, 2); // 15s, 50ms interval
        // Fase 2: Levendige kleurloop met felle kleuren (15s)
        strip.runningOppositeNoNeighbors(13 * 1000); // 15s, 50ms interval

        // Fase 3: Engelen zingen, motor start langzaam op (10s)
        for (int speed = 0; speed <= 20; ++speed) {
            motors.getAngelMotor().setSpeed(speed);
            wait(100); // 2s opstarten
        }

        // swelling light that modulates in brightness and warmth, like angels singing, for 6 seconds
        const int swellDurationMs = 6000;
        const int swellSteps = 60;
        for (int step = 0; step < swellSteps; ++step) {
            float progress = static_cast<float>(step) / static_cast<float>(swellSteps);
            // Brightness oscillates between 50% and 100%
            int brightness = static_cast<int>(127.5f * (1.0f + sinf(progress * 2.0f * M_PI * 3.0f))); // 3 full cycles
            // Warmth oscillates between 0 (cool) and 1 (warm)
            float warmth = 0.5f * (1.0f + sinf(progress * 2.0f * M_PI * 2.0f + M_PI / 2.0f)); // 2 full cycles

            for (int i = 0; i < strip.numLEDs; ++i) {
                // Base color is a warm white (e.g., RGB(255, 223, 191))
                int r = static_cast<int>(255 * (0.8f + 0.2f * warmth));
                int g = static_cast<int>(223 * (0.8f + 0.2f * warmth));
                int b = static_cast<int>(191 * (0.8f + 0.2f * warmth));
                strip.setLed(i, std::make_tuple(r, g, b), brightness, false);
            }
            strip.refresh();
            wait(swellDurationMs / swellSteps);
        }

        for (int speed = 20; speed >= 0; --speed) {
            motors.getAngelMotor().setSpeed(speed);
            wait(100); // 2s afremmen
        }
        motors.getAngelMotor().stop();
        motors.getTreeMotor().setSpeed(10); // Boom motor 5s draaien

        // Fase 4: Fireworks effect (7s)
        for (int i = 0; i < 5; ++i) {
            strip.fireworks(1000); // 3 foci, 50 steps, 30ms per step ≈ 1.5s per firework
            wait(1500);
        }

        // Fase 5: Alles langzaam uitfaden (3s)
        for (int b = 255; b >= 0; b -= 8) {
            for (int i = 0; i < strip.numLEDs; ++i) {
                auto last = strip.getColor(i);
                strip.setLed(i, last, b, false);
            }
            strip.refresh();
            wait(30); // 32 stappen * 30ms ≈ 1s, maar fade kan langer duren
        }
        strip.turnOff();

        stop();
    }

private:
    // HSV to RGB helper for kleurloops
    std::tuple<int, int, int> hsvToRgb(float h, float s, float v)
    {
        float c = v * s;
        float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
        float m = v - c;
        float r, g, b;
        if (h < 60) {
            r = c;
            g = x;
            b = 0;
        } else if (h < 120) {
            r = x;
            g = c;
            b = 0;
        } else if (h < 180) {
            r = 0;
            g = c;
            b = x;
        } else if (h < 240) {
            r = 0;
            g = x;
            b = c;
        } else if (h < 300) {
            r = x;
            g = 0;
            b = c;
        } else {
            r = c;
            g = 0;
            b = x;
        }
        return std::make_tuple(
            static_cast<int>((r + m) * 255), static_cast<int>((g + m) * 255), static_cast<int>((b + m) * 255));
    }
};

#endif // HERDERTJES_SCENE_HPP
