#ifndef DFPLAYER_HPP
#define DFPLAYER_HPP

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define DF_UART_NUM UART_NUM_1
#define DF_RX GPIO_NUM_14 // ESP32 receives FROM DFPlayer TX
#define DF_TX GPIO_NUM_13 // ESP32 sends TO DFPlayer RX
#define BUF_SIZE 1024

class DFPlayer {
private:
    uart_port_t uart_num;
    static const char* TAG;

    void sendCommand(uint8_t cmd[], size_t len)
    {
        uart_write_bytes(uart_num, (const char*)cmd, len);
        vTaskDelay(pdMS_TO_TICKS(100)); // Arduino library uses longer delays
    }

    void sendCommand(uint8_t command, uint8_t param1 = 0x00, uint8_t param2 = 0x00)
    {
        uint8_t cmd[10];
        cmd[0] = 0x7E;
        cmd[1] = 0xFF;
        cmd[2] = 0x06;
        cmd[3] = command;
        cmd[4] = 0x01; // Request feedback (Arduino library default) 0x00 is feedback
        cmd[5] = param1;
        cmd[6] = param2;

        // Calculate proper checksum
        int16_t checksum = -(cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5] + cmd[6]);
        cmd[7] = (uint8_t)(checksum >> 8);
        cmd[8] = (uint8_t)(checksum & 0xFF);
        cmd[9] = 0xEF;

        ESP_LOGI(TAG, "Sending command: 0x%02X, params: 0x%02X 0x%02X", command, param1, param2);
        sendCommand(cmd, sizeof(cmd));
    }

public:
    DFPlayer(uart_port_t uart = DF_UART_NUM)
        : uart_num(uart)
    {
    }

    void begin()
    {
        ESP_LOGI(TAG, "Initializing DFPlayer...");

        // UART setup - exactly like Arduino
        uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT,
            .flags = { 0 },
        };

        ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(uart_num, DF_TX, DF_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));

        // Match Arduino library timing
        vTaskDelay(pdMS_TO_TICKS(500));

        // Send initial reset like Arduino library does
        // reset();
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "DFPlayer initialized");
    }

    void play()
    {
        sendCommand(0x0D);
        ESP_LOGI(TAG, "Play");
    }

    void stop()
    {
        sendCommand(0x16);
        ESP_LOGI(TAG, "Stop");
    }

    void pause()
    {
        sendCommand(0x0E);
        ESP_LOGI(TAG, "Pause");
    }

    void next()
    {
        sendCommand(0x01);
        ESP_LOGI(TAG, "Next");
    }

    void previous()
    {
        sendCommand(0x02);
        ESP_LOGI(TAG, "Previous");
    }

    void playTrack(uint16_t track)
    {
        sendCommand(0x03, track >> 8, track & 0xFF);
        ESP_LOGI(TAG, "Playing track %d", track);
    }

    void setVolume(uint8_t vol)
    {
        if (vol > 30)
            vol = 30;
        sendCommand(0x06, 0x00, vol);
        ESP_LOGI(TAG, "Volume: %d", vol);
    }

    void reset()
    {
        sendCommand(0x0C);
        ESP_LOGI(TAG, "Reset");
    }

    void sleep() { sendCommand(0x0A); }
    void wakeUp() { sendCommand(0x0B); }
};

const char* DFPlayer::TAG = "DFPlayer";

#endif // DFPLAYER_HPP
