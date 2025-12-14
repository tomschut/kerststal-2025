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

protected:
    Lights& strip;
    DFPlayer& player;
    Motors& motors;
    void stop()
    {
        player.stop();
        player.setVolume(20);
        strip.turnOff();
        motors.stopAll(); // if needed
        wait(500);
    }
};

#endif // SCENE_HPP
