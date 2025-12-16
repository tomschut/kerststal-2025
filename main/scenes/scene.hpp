#ifndef SCENE_HPP
#define SCENE_HPP

#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"

class Scene {
public:
    virtual ~Scene() = default;
    virtual void play() = 0;
    Scene(Lights& strip, DFPlayer& dfplayer, Motors& motors)
        : strip(strip)
        , player(dfplayer)
        , motors(motors)
    {
    }
    void stop()
    {
        // Fade out volume
        for (int vol = player.getVolume(); vol > 0; vol -= 2) {
            player.setVolume(vol);
            wait(50); // small delay for smooth fade
        }
        player.stop();

        // Fade out lights
        for (int brightness = 100; brightness >= 0; brightness -= 10) {
            strip.setBrightness(brightness); // You need to implement setBrightness in Lights
            wait(30);
        }
        strip.turnOff();

        // Smoothly slow all running motors to a stop
        bool anyRunning;
        do {
            anyRunning = false;
            for (int i = 0; i < 4; ++i) {
                Motor& motor = motors.getMotor(i);
                int currentSpeed = motor.getSpeed();
                if (currentSpeed > 0) {
                    motor.setSpeed(currentSpeed - 5);
                    anyRunning = true;
                } else if (currentSpeed < 0) {
                    motor.setSpeed(currentSpeed + 5);
                    anyRunning = true;
                }
            }
            wait(100);
        } while (anyRunning);

        wait(500);
    }

protected:
    Lights& strip;
    DFPlayer& player;
    Motors& motors;
};

#endif // SCENE_HPP
