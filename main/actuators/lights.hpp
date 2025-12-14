#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include "../util.hpp" // for wait function
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include <cmath>
#include <esp_random.h>
#include <tuple>
#include <vector>

static const char* TAG = "LEDStrip";

class Lights {
public:
    int numLEDs;
    Lights(int numLEDs, gpio_num_t dataPin)
        : numLEDs(numLEDs)
        , dataPin(dataPin)
        , strip_handle(nullptr)
    {
        // Configure LED strip with RMT peripheral
        led_strip_config_t strip_config = { .strip_gpio_num = dataPin,
            .max_leds = static_cast<uint32_t>(numLEDs),
            .led_model = LED_MODEL_WS2812,
            .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
            .flags = { .invert_out = false } };

        led_strip_rmt_config_t rmt_config = { .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 10 * 1000 * 1000, // 10MHz resolution
            .mem_block_symbols = 0,
            .flags = { .with_dma = false } };

        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip_handle));
        led_strip_clear(strip_handle);

        ESP_LOGI(TAG, "LED strip initialized on GPIO %d with %d LEDs", dataPin, numLEDs);
    }

    ~Lights()
    {
        if (strip_handle) {
            led_strip_del(strip_handle);
        }
    }

    void turnOff()
    {
        led_strip_clear(strip_handle);
        led_strip_refresh(strip_handle);
        // ESP_LOGI(TAG, "LED strip turned off");
    }

    void setLed(int index, std::tuple<uint8_t, uint8_t, uint8_t> color, int brightness = 255, bool refresh = true)
    {
        if (!strip_handle) {
            ESP_LOGE(TAG, "strip_handle is null!");
            return;
        }
        if (index < 0 || index >= numLEDs) {
            ESP_LOGW(TAG, "LED index %d out of bounds (0-%d)", index, numLEDs - 1);
            return;
        }
        // Clamp brightness to 0-255
        if (brightness < 0)
            brightness = 0;
        if (brightness > 255)
            brightness = 255;

        uint8_t r = (std::get<0>(color) * brightness) / 255;
        uint8_t g = (std::get<1>(color) * brightness) / 255;
        uint8_t b = (std::get<2>(color) * brightness) / 255;

        led_strip_set_pixel(strip_handle, index, r, g, b);

        if (refresh) {
            led_strip_refresh(strip_handle);
        }
    }

    void setMultipleLeds(int from, int to, const std::tuple<uint8_t, uint8_t, uint8_t>& color, int brightness = 255)
    {
        for (int i = from; i <= to; ++i) {
            setLed(i, color, brightness, false);
        }
        refresh();
    }

    void refresh() { led_strip_refresh(strip_handle); }

    void sparkeMultipleLeds(int duration_ms, int interval_ms)
    {
        {
            int elapsed = 0;
            int pickCount = std::min(30, numLEDs);
            while (elapsed < duration_ms) {
                // pick unique random indices
                std::vector<int> picks;
                picks.reserve(pickCount);
                while ((int)picks.size() < pickCount) {
                    int idx = esp_random() % numLEDs;
                    bool found = false;
                    for (int p : picks) {
                        if (p == idx) {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        picks.push_back(idx);
                }

                // set picked LEDs to random colors (no immediate refresh)
                for (int idx : picks) {
                    auto color = std::make_tuple(static_cast<uint8_t>(esp_random() % 256),
                        static_cast<uint8_t>(esp_random() % 256), static_cast<uint8_t>(esp_random() % 256));
                    setLed(idx, color, 255, false);
                }
                refresh();

                // keep them on for interval
                wait(interval_ms);

                // turn picked LEDs off
                for (int idx : picks) {
                    setLed(idx, std::make_tuple(0, 0, 0), 0, false);
                }
                refresh();

                elapsed += interval_ms;
            }
        }
    }

    void beatDrop(int from, int to, int drop_duration_ms = 1000)
    {
        {
            // clamp range
            if (from < 0)
                from = 0;
            if (to >= numLEDs)
                to = numLEDs - 1;
            if (from > to)
                return;

            int range = to - from + 1;
            if (range <= 0)
                return;

            const int maxInterval = 200; // starting flicker interval (ms)
            const int minInterval = 10; // fastest flicker before the drop (ms)

            // Make the cycle speed up each iteration by multiplying the interval
            float currentInterval = static_cast<float>(maxInterval);
            const float decayFactor = 0.80f; // 20% faster each cycle (tweak to taste)

            int elapsed = 0;
            while (elapsed < drop_duration_ms) {
                int interval = std::max(minInterval, static_cast<int>(currentInterval));

                // choose a small random subset to flicker this cycle
                int pickCount
                    = std::max(1, std::min(range, static_cast<int>(1 + (esp_random() % (std::max(1, range / 4))))));
                std::vector<int> picks;
                picks.reserve(pickCount);
                while ((int)picks.size() < pickCount) {
                    int idx = from + (esp_random() % range);
                    bool found = false;
                    for (int p : picks) {
                        if (p == idx) {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        picks.push_back(idx);
                }

                // turn picked LEDs on with random warm-ish colors (no immediate refresh)
                for (int idx : picks) {
                    auto color = std::make_tuple<uint8_t, uint8_t, uint8_t>(
                        static_cast<uint8_t>(200 + (esp_random() % 56)), // R: 200-255
                        static_cast<uint8_t>(100 + (esp_random() % 156)), // G: 100-255
                        static_cast<uint8_t>(0 + (esp_random() % 80)) // B: 0-79
                    );
                    setLed(idx, color, 255, false);
                }
                refresh();

                wait(interval / 2);

                // turn picked LEDs off
                for (int idx : picks) {
                    setLed(idx, std::make_tuple<uint8_t, uint8_t, uint8_t>(0, 0, 0), 0, false);
                }
                refresh();

                wait(interval / 2);

                elapsed += interval;

                // speed up for next cycle
                currentInterval = currentInterval * decayFactor;
                if (currentInterval < static_cast<float>(minInterval))
                    currentInterval = static_cast<float>(minInterval);
            }

            // final "drop" — quick sequential turn-off sweep across the range
            const int sweepDelay = std::max(5, minInterval / 2);
            for (int i = from; i <= to; ++i) {
                setLed(i, std::make_tuple<uint8_t, uint8_t, uint8_t>(0, 0, 0), 0, false);
                refresh();
                wait(sweepDelay);
            }
            // ensure all are off
            turnOff();
        }
    }

    void lightning(int from, int to, int flashes = 3, int flash_duration_ms = 100, int pause_duration_ms = 300)
    {
        for (int i = 0; i < flashes; ++i) {
            // Full brightness white
            for (int j = from; j <= to; ++j) {
                setLed(j, std::make_tuple(255, 255, 255), 255, false);
            }
            refresh();
            vTaskDelay(pdMS_TO_TICKS(flash_duration_ms));

            // Turn off
            turnOff();
            vTaskDelay(pdMS_TO_TICKS(pause_duration_ms));
        }
    }

    // all leds pulsing like a chaos, random colors at random positions, multiple leds, on pulse, it gives the
    // all leds turn off and on in between pulses
    void pulsingChaos(int timeInMs, int pulseIntervalMs = 400)
    {
        int elapsed = 0;
        while (elapsed < timeInMs) {
            int numPulses = esp_random() % 5 + 3; // 3 to 7 pulses
            for (int i = 0; i < numPulses; ++i) {
                // Turn all LEDs on with random colors
                for (int j = 0; j < numLEDs; ++j) {
                    auto color = std::make_tuple(esp_random() % 256, esp_random() % 256, esp_random() % 256);
                    setLed(j, color, 255, false);
                }
                refresh();
                wait(pulseIntervalMs / 2);
                turnOff();
                wait(pulseIntervalMs / 2);
            }
            elapsed += pulseIntervalMs * numPulses;
        }
    }

    void ambientGlow(int steps = 100, int step_delay_ms = 50)
    {
        // Fade in while changing colors
        for (int b = 0; b <= 255; b += (256 / steps)) {
            for (int i = 0; i < numLEDs; ++i) {
                float hue = (360.0f * b) / 255.0f;
                auto color = hsv2rgb(hue, 0.8f, 1.0f);
                setLed(i, color, b, false);
            }
            refresh();
            wait(step_delay_ms);
        }

        // Fade out while changing colors
        for (int b = 255; b >= 0; b -= (256 / steps)) {
            for (int i = 0; i < numLEDs; ++i) {
                float hue = (360.0f * b) / 255.0f;
                auto color = hsv2rgb(hue, 0.8f, 1.0f);
                setLed(i, color, b, false);
            }
            refresh();
            wait(step_delay_ms);
        }
    }

    void pulsingBeat(int timeInMs, int pulseIntervalMs = 400)
    {
        int elapsed = 0;
        while (elapsed < timeInMs) {
            // Pick a random color for this beat
            auto color = std::make_tuple(esp_random() % 256, esp_random() % 256, esp_random() % 256);

            // Fade in
            for (int b = 0; b <= 255; b += 16) {
                for (int j = 0; j < numLEDs; ++j) {
                    setLed(j, color, b, false);
                }
                refresh();
                wait(pulseIntervalMs / 32); // Adjust for smoothness
            }

            // Fade out
            for (int b = 255; b >= 0; b -= 16) {
                for (int j = 0; j < numLEDs; ++j) {
                    setLed(j, color, b, false);
                }
                refresh();
                wait(pulseIntervalMs / 32);
            }

            elapsed += pulseIntervalMs;
        }
        turnOff();
    }

    void pulsingBeatInSections(int timeInMs, int n_sections, int pulseIntervalMs = 400)
    {
        if (n_sections <= 1) {
            // fallback to pulsing whole strip if less than 2 sections requested
            pulsingBeat(timeInMs, pulseIntervalMs);
            return;
        }

        int elapsed = 0;

        // helper to compute section start/end (end is exclusive)
        auto section_range = [this, n_sections](int section) -> std::pair<int, int> {
            int start = (numLEDs * section) / n_sections;
            int end = (numLEDs * (section + 1)) / n_sections;
            if (start < 0) start = 0;
            if (end > numLEDs) end = numLEDs;
            return {start, end};
        };

        while (elapsed < timeInMs) {
            for (int section = 0; section < n_sections; ++section) {
                // pick the second section to always have two sections on (wrap-around)
                int otherSection = (section + 1) % n_sections;

                // Pick a random color for this beat (same color for both sections)
                auto color = std::make_tuple(static_cast<uint8_t>(esp_random() % 256),
                                             static_cast<uint8_t>(esp_random() % 256),
                                             static_cast<uint8_t>(esp_random() % 256));

                // Fade in
                for (int b = 0; b <= 255; b += 16) {
                    // Turn all sections off except the two active ones
                    for (int s = 0; s < n_sections; ++s) {
                        if (s == section || s == otherSection)
                            continue;
                        auto r = section_range(s);
                        for (int j = r.first; j < r.second; ++j) {
                            setLed(j, std::make_tuple<uint8_t, uint8_t, uint8_t>(0, 0, 0), 0, false);
                        }
                    }

                    // Pulse the two active sections
                    auto r1 = section_range(section);
                    for (int j = r1.first; j < r1.second; ++j) {
                        setLed(j, color, b, false);
                    }
                    auto r2 = section_range(otherSection);
                    for (int j = r2.first; j < r2.second; ++j) {
                        setLed(j, color, b, false);
                    }

                    refresh();
                    wait(pulseIntervalMs / 32);
                }

                // Fade out
                for (int b = 255; b >= 0; b -= 16) {
                    // Turn all sections off except the two active ones
                    for (int s = 0; s < n_sections; ++s) {
                        if (s == section || s == otherSection)
                            continue;
                        auto r = section_range(s);
                        for (int j = r.first; j < r.second; ++j) {
                            setLed(j, std::make_tuple<uint8_t, uint8_t, uint8_t>(0, 0, 0), 0, false);
                        }
                    }

                    // Pulse the two active sections
                    auto r1 = section_range(section);
                    for (int j = r1.first; j < r1.second; ++j) {
                        setLed(j, color, b, false);
                    }
                    auto r2 = section_range(otherSection);
                    for (int j = r2.first; j < r2.second; ++j) {
                        setLed(j, color, b, false);
                    }

                    refresh();
                    wait(pulseIntervalMs / 32);
                }

                elapsed += pulseIntervalMs;
            }
        }
        turnOff();
    }

private:
    gpio_num_t dataPin;
    led_strip_handle_t strip_handle;

    // HSV to RGB conversion (h in [0, 360), s and v in [0, 1])
    static std::tuple<uint8_t, uint8_t, uint8_t> hsv2rgb(float h, float s, float v)
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
        return std::make_tuple(static_cast<uint8_t>((r + m) * 255), static_cast<uint8_t>((g + m) * 255),
            static_cast<uint8_t>((b + m) * 255));
    }
};

#endif // NEOPIXEL_HELPER_HPP
