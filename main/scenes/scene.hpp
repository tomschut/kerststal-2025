#include "../actuators/dfplayer.hpp"
#include "../actuators/lights.hpp"

class Scene {
public:
    virtual ~Scene() = default;
    virtual void play() = 0;
    Scene(Lights& strip, DFPlayer& dfplayer)
        : strip(strip)
        , dfplayer(dfplayer)
    {
    }

protected:
    Lights& strip;
    DFPlayer& dfplayer;
};
