#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include <tuple>
#include <vector>

static const char* TAG = "LEDStrip";

class Lights {
public:
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
        ESP_LOGI(TAG, "LED strip turned off");
    }

    void setLed(int index, std::tuple<uint8_t, uint8_t, uint8_t> color, bool refresh = true)
    {
        if (index < 0 || index >= numLEDs) {
            ESP_LOGW(TAG, "LED index %d out of bounds (0-%d)", index, numLEDs - 1);
            return;
        }
        led_strip_set_pixel(strip_handle, index, std::get<0>(color), std::get<1>(color), std::get<2>(color));
        led_strip_refresh(strip_handle); // Actually send data to LEDs
    }

    void setLeds(int start, int end, std::tuple<uint8_t, uint8_t, uint8_t> color)
    {
        if (start < 0 || start >= numLEDs || end < 0 || end >= numLEDs || start > end) {
            ESP_LOGW(TAG, "LED index %d out of bounds (0-%d)", start, numLEDs - 1);
            return;
        }
        for (int i = start; i <= end; ++i) {
            this->setLed(i, color, false);
        }
        led_strip_refresh(strip_handle);
    }

private:
    int numLEDs;
    gpio_num_t dataPin;
    led_strip_handle_t strip_handle;
};

#endif // NEOPIXEL_HELPER_HPP
