// class that combines all motors, gets the pins in, and creates them
#ifndef MOTORS_HPP
#define MOTORS_HPP
#include "driver/gpio.h"
#include "driver/mcpwm.h"
#include "motor.hpp"
#include <array>

class Motors {
public:
    Motors(const std::array<gpio_num_t, 4>& pins)
        : motors { Motor(pins[0], LEDC_CHANNEL_0), Motor(pins[1], LEDC_CHANNEL_1), Motor(pins[2], LEDC_CHANNEL_2),
            Motor(pins[3], LEDC_CHANNEL_3) }
    {
    }

    Motor& getMotor(int index) { return motors[index]; }

    Motor& getShepherdMotor() { return motors[3]; }
    Motor& getTreeMotor() { return motors[2]; }
    Motor& getNativityMotor() { return motors[1]; }
    Motor& getAngelMotor() { return motors[0]; }

    void stopAll()
    {
        for (auto& motor : motors) {
            motor.stop();
        }
    }

private:
    std::array<Motor, 4> motors;
};

#endif // MOTORS_HPP
