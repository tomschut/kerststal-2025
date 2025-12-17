#ifndef MOTOR_HPP
#define MOTOR_HPP
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define MOTOR_SPEED_FORWARD 2000 // duty voor volle snelheid vooruit
#define MOTOR_SPEED_STOP 1455 // duty voor stop
#define MOTOR_SPEED_BACK 1000 // duty voor volle snelheid achteruit

class Motor {
private:
    int motorPin;
    ledc_channel_t channel_;

    void setMotorDuty(uint32_t duty_us)
    {
        uint32_t duty = (duty_us * ((1 << 13) - 1)) / 20000; // 20ms periode
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel_, duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel_);
    }

    void setupMotorPWM()
    {
        ledc_timer_config_t timer_conf = {};
        timer_conf.duty_resolution = LEDC_TIMER_13_BIT;
        timer_conf.freq_hz = 50; // servo standaard 50Hz
        timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
        timer_conf.timer_num = LEDC_TIMER_0;
        ledc_timer_config(&timer_conf);

        ledc_channel_config_t channel_conf = {};
        channel_conf.channel = channel_;
        channel_conf.duty = 0;
        channel_conf.gpio_num = motorPin;
        channel_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
        channel_conf.hpoint = 0;
        channel_conf.timer_sel = LEDC_TIMER_0;
        ledc_channel_config(&channel_conf);
    }

public:
    Motor(int pin, ledc_channel_t channel)
        : motorPin(pin)
        , channel_(channel)
    {
        setupMotorPWM();
    }

    void stop()
    {
        this->speed = 0;
        setMotorDuty(MOTOR_SPEED_STOP);
    }

    void setSpeed(int speed, uint32_t duration_ms = 0)
    {
        this->speed = speed;
        // Clamp speed tussen -100 en 100
        if (speed > 100)
            speed = 100;
        if (speed < -100)
            speed = -100;

        if (speed == 0) {
            setMotorDuty(MOTOR_SPEED_STOP);
            return;
        }
        // Map speed (-100 tot 100) naar duty cycle (1000 tot 2000 us)
        double duty_us
            = MOTOR_SPEED_STOP + (static_cast<double>(speed) * (MOTOR_SPEED_FORWARD - MOTOR_SPEED_STOP) / 100.0);

        setMotorDuty(static_cast<uint32_t>(duty_us));

        // Als duration is opgegeven, stop na die tijd
        if (duration_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(duration_ms));
            setMotorDuty(MOTOR_SPEED_STOP);
        }
    }

    int getSpeed() const { return speed; }

private:
    int speed = 0; // huidige snelheid instelling
};

#endif // MOTOR_HPP
