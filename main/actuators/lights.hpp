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

class Lights {
public:
    const char* TAG = "LEDStrip";
    int numLEDs;
    Lights(int numLEDs, gpio_num_t dataPin)
        : numLEDs(numLEDs)
        , dataPin(dataPin)
        , strip_handle(nullptr)
        , lastColors(numLEDs, std::make_tuple(0, 0, 0))
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

        // Ensure all LEDs are off at startup
        turnOff();
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

    void setBrightness(int brightness)
    {
        // Clamp brightness to 0-255
        if (brightness < 0)
            brightness = 0;
        if (brightness > 255)
            brightness = 255;
        this->brightness = brightness;
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
        lastColors[index] = color;

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
            if (start < 0)
                start = 0;
            if (end > numLEDs)
                end = numLEDs;
            return { start, end };
        };

        while (elapsed < timeInMs) {
            for (int section = 0; section < n_sections; ++section) {
                // pick the second section to always have two sections on (wrap-around)
                int otherSection = (section + 1) % n_sections;

                // Pick a random color for this beat (same color for both sections)
                auto color = std::make_tuple(static_cast<uint8_t>(esp_random() % 256),
                    static_cast<uint8_t>(esp_random() % 256), static_cast<uint8_t>(esp_random() % 256));

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

    void runningLights(int timeInMs, int speedMs = 100)
    {
        int elapsed = 0;
        while (elapsed < timeInMs) {
            for (int i = 0; i < numLEDs; ++i) {
                // Turn all LEDs off
                for (int j = 0; j < numLEDs; ++j) {
                    setLed(j, std::make_tuple(0, 0, 0), 0, false);
                }
                // Turn on the current LED with a bright color
                auto color = std::make_tuple(static_cast<uint8_t>(esp_random() % 256),
                    static_cast<uint8_t>(esp_random() % 256), static_cast<uint8_t>(esp_random() % 256));
                setLed(i, color, 255, false);
                refresh();
                wait(speedMs);
                elapsed += speedMs;
                if (elapsed >= timeInMs)
                    break;
            }
        }
        turnOff();
    }

    // method that displays fireworks, it moves the leds from two sides to multiple focal points getting brighter until
    // the "explosion" at the focal points, and then move back again getting less bright, the focal points are chosen
    // randomly
    void fireworks(int timeInMs, int n_foci = 3, int travel_time_ms = 800, int explosion_duration_ms = 400)
    {
        if (numLEDs <= 2)
            return;

        if (n_foci <= 0)
            n_foci = 1;
        n_foci = std::min(n_foci, std::max(1, numLEDs / 4));

        int elapsed = 0;
        const int frame_ms = 25; // base frame time for smoother motion

        while (elapsed < timeInMs) {
            // pick unique focal points, spaced somewhat apart
            std::vector<int> foci;
            while ((int)foci.size() < n_foci) {
                int candidate = esp_random() % numLEDs;
                bool ok = true;
                for (int f : foci) {
                    if (abs(f - candidate) < std::max(3, numLEDs / 40)) {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                    foci.push_back(candidate);
            }

            // assign each focus a warm-ish color
            std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> colors;
            for (int i = 0; i < (int)foci.size(); ++i) {
                // warm colors toward yellow/white
                uint8_t r = 200 + (esp_random() % 56);
                uint8_t g = 120 + (esp_random() % 136);
                uint8_t b = esp_random() % 80;
                colors.emplace_back(r, g, b);
            }

            // inbound: comets move from both ends toward each focus
            int steps_in = std::max(3, travel_time_ms / frame_ms);
            for (int step = 0; step <= steps_in && elapsed < timeInMs; ++step) {
                float t = static_cast<float>(step) / static_cast<float>(steps_in);
                // clear with very low residual to keep trails from previous frames
                for (int i = 0; i < numLEDs; ++i) {
                    // slightly dim previous color to create fade (do not rely on lastColors alone)
                    setLed(i, std::make_tuple(0, 0, 0), 0, false);
                }

                for (size_t fi = 0; fi < foci.size(); ++fi) {
                    int focus = foci[fi];
                    int leftStart = 0;
                    int rightStart = numLEDs - 1;

                    int leftPos = static_cast<int>(leftStart + t * (focus - leftStart));
                    int rightPos = static_cast<int>(rightStart + t * (focus - rightStart));

                    // draw head and a short fading trail for both comets
                    const int trail = 5;
                    for (int tr = 0; tr < trail; ++tr) {
                        float trailFactor = (trail - tr) / static_cast<float>(trail);
                        int lp = leftPos - tr;
                        int rp = rightPos + tr;
                        int brightness
                            = static_cast<int>(255 * trailFactor * (0.3f + 0.7f * t)); // brighten toward focus
                        if (lp >= 0 && lp < numLEDs) {
                            auto col = colors[fi];
                            setLed(lp, col, brightness, false);
                        }
                        if (rp >= 0 && rp < numLEDs) {
                            auto col = colors[fi];
                            setLed(rp, col, brightness, false);
                        }
                    }
                }

                refresh();
                wait(frame_ms);
                elapsed += frame_ms;
            }

            if (elapsed >= timeInMs)
                break;

            // explosion: expand bright burst then contract with a few flickers
            const int explosion_steps = std::max(3, explosion_duration_ms / frame_ms);
            for (int phase = 0; phase < 2 && elapsed < timeInMs; ++phase) {
                // phase 0: expand, phase 1: contract
                for (int s = 0; s <= explosion_steps && elapsed < timeInMs; ++s) {
                    float t = static_cast<float>(s) / static_cast<float>(explosion_steps);
                    int maxRadius = std::min(12, numLEDs / 6);
                    int radius = static_cast<int>(t * maxRadius);
                    for (int i = 0; i < numLEDs; ++i) {
                        setLed(i, std::make_tuple(0, 0, 0), 0, false);
                    }
                    for (size_t fi = 0; fi < foci.size(); ++fi) {
                        int focus = foci[fi];
                        auto col = colors[fi];
                        for (int off = -radius; off <= radius; ++off) {
                            int idx = focus + off;
                            if (idx < 0 || idx >= numLEDs)
                                continue;
                            float distFactor = 1.0f - (std::abs(off) / static_cast<float>(std::max(1, radius)));
                            // pulsate brightness and add small random flicker
                            int flick = (esp_random() % 40) - 20;
                            int brightness = static_cast<int>((200 * distFactor + 55 * t) + flick);
                            brightness = std::max(0, std::min(255, brightness));
                            setLed(idx, col, brightness, false);
                        }
                    }
                    refresh();
                    wait(frame_ms);
                    elapsed += frame_ms;
                }
            }

            if (elapsed >= timeInMs)
                break;

            // outbound: comets return to their ends, fading out
            int steps_out = std::max(3, travel_time_ms / frame_ms);
            for (int step = 0; step <= steps_out && elapsed < timeInMs; ++step) {
                float t = static_cast<float>(step) / static_cast<float>(steps_out);
                // t goes 0..1, map to positions moving from focus back to ends
                for (int i = 0; i < numLEDs; ++i) {
                    setLed(i, std::make_tuple(0, 0, 0), 0, false);
                }
                for (size_t fi = 0; fi < foci.size(); ++fi) {
                    int focus = foci[fi];
                    int leftEnd = 0;
                    int rightEnd = numLEDs - 1;
                    int leftPos = static_cast<int>(focus + t * (leftEnd - focus));
                    int rightPos = static_cast<int>(focus + t * (rightEnd - focus));
                    const int trail = 5;
                    for (int tr = 0; tr < trail; ++tr) {
                        float trailFactor = (trail - tr) / static_cast<float>(trail);
                        int lp = leftPos + tr; // trail away from focus
                        int rp = rightPos - tr;
                        int brightness = static_cast<int>(255 * trailFactor * (1.0f - t)); // fade as they leave
                        if (lp >= 0 && lp < numLEDs) {
                            auto col = colors[fi];
                            setLed(lp, col, brightness, false);
                        }
                        if (rp >= 0 && rp < numLEDs) {
                            auto col = colors[fi];
                            setLed(rp, col, brightness, false);
                        }
                    }
                }
                refresh();
                wait(frame_ms);
                elapsed += frame_ms;
            }

            // small pause between rockets
            wait(120);
            elapsed += 120;
        }

        turnOff();
    }

    // method that draws people in while ambient is playing
    void beckon()
    {
        // Light up LEDs one by one from both ends to the center
        for (int i = 0; i < numLEDs / 2; ++i) {
            auto color = std::make_tuple(0, 0, 255); // Blue color
            setLed(i, color, 255, false);
            setLed(numLEDs - 1 - i, color, 255, false);
            refresh();
            wait(100); // Delay between lighting up each pair
        }

        // Hold the full strip lit for a moment
        wait(500);

        // Turn off LEDs one by one from center to both ends
        for (int i = numLEDs / 2 - 1; i >= 0; --i) {
            setLed(i, std::make_tuple(0, 0, 0), 0, false);
            setLed(numLEDs - 1 - i, std::make_tuple(0, 0, 0), 0, false);
            refresh();
            wait(100); // Delay between turning off each pair
        }
    }

    // Returns the last set color for a given LED index
    std::tuple<uint8_t, uint8_t, uint8_t> getColor(int index) const
    {
        if (index < 0 || index >= numLEDs) {
            return std::make_tuple(0, 0, 0);
        }
        return lastColors[index];
    }

    void runningOppositeNoNeighbors(int durationMs = 5000, int speedMs = 120, int runners = 4)
    {
        // runners: number of lights in each direction (total = runners*2)
        // Each runner has a position and direction (+1 or -1)
        struct Runner {
            int pos;
            int dir;
            std::tuple<uint8_t, uint8_t, uint8_t> color;
        };

        std::vector<Runner> all;
        // Left-to-right
        for (int i = 0; i < runners; ++i) {
            all.push_back({ i * (numLEDs / (runners + 1)), 1, std::make_tuple(255, 180 - 60 * i, 60 + 80 * i) });
        }
        // Right-to-left
        for (int i = 0; i < runners; ++i) {
            all.push_back(
                { numLEDs - 1 - i * (numLEDs / (runners + 1)), -1, std::make_tuple(60 + 80 * i, 180 - 60 * i, 255) });
        }

        int elapsed = 0;
        std::vector<int> prevPos(all.size(), -2);

        while (elapsed < durationMs) {
            // Clear strip
            for (int i = 0; i < numLEDs; ++i)
                setLed(i, std::make_tuple(0, 0, 0), 0, false);

            // Mark occupied positions to avoid neighbors
            std::vector<bool> occupied(numLEDs, false);
            for (auto& r : all)
                if (r.pos >= 0 && r.pos < numLEDs)
                    occupied[r.pos] = true;

            // Draw runners
            for (size_t i = 0; i < all.size(); ++i) {
                auto& r = all[i];
                if (r.pos >= 0 && r.pos < numLEDs)
                    setLed(r.pos, r.color, 255, false);
            }
            refresh();

            // Move runners, skipping if next pos is occupied
            for (size_t i = 0; i < all.size(); ++i) {
                auto& r = all[i];
                int next = r.pos + r.dir;
                if (next >= 0 && next < numLEDs && !occupied[next]
                    && (next - r.pos) * r.dir == 1) { // not skipping over
                    prevPos[i] = r.pos;
                    r.pos = next;
                } else {
                    // If blocked or at end, bounce back
                    r.dir = -r.dir;
                    r.pos += r.dir;
                    // Prevent overlap after bounce
                    if (r.pos >= 0 && r.pos < numLEDs && occupied[r.pos])
                        r.pos -= r.dir;
                }
            }

            wait(speedMs);
            elapsed += speedMs;
        }
        turnOff();
    }

private:
    gpio_num_t dataPin;
    led_strip_handle_t strip_handle;
    int brightness = 0;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> lastColors;

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
