#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <stdio.h>

#define MOTOR_PIN GPIO_NUM_32
#define MOTOR_SPEED_FORWARD 2000  // duty voor volle snelheid vooruit
#define MOTOR_SPEED_STOP    1500  // duty voor stop
#define MOTOR_SPEED_BACK    1000  // duty voor volle snelheid achteruit

void setupMotorPWM()
{
    ledc_timer_config_t timer_conf = {};
    timer_conf.duty_resolution = LEDC_TIMER_16_BIT;
    timer_conf.freq_hz = 50;  // servo standaard 50Hz
    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {};
    channel_conf.channel    = LEDC_CHANNEL_0;
    channel_conf.duty       = 0;
    channel_conf.gpio_num   = MOTOR_PIN;
    channel_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_conf.hpoint     = 0;
    channel_conf.timer_sel  = LEDC_TIMER_0;
    ledc_channel_config(&channel_conf);
}

void setMotorDuty(uint32_t duty_us)
{
    // LEDC werkt met duty resolution bits
    uint32_t duty = (duty_us * ((1 << 16) - 1)) / 20000; // 20ms periode
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

extern "C" void app_main()
{
    printf("FS90R test gestart\n");
    setupMotorPWM();

    while (true)
    {
        printf("Vooruit draaien\n");
        setMotorDuty(MOTOR_SPEED_FORWARD);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("Stop\n");
        setMotorDuty(MOTOR_SPEED_STOP);
        vTaskDelay(pdMS_TO_TICKS(1000));

        printf("Achteruit draaien\n");
        setMotorDuty(MOTOR_SPEED_BACK);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("Stop\n");
        setMotorDuty(MOTOR_SPEED_STOP);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
