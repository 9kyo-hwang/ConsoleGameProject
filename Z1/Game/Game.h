#pragma once

#include <Engine/Engine.h>
#include <vector>

namespace Craft
{
    class Level;
}

enum class State
{
    Title,
    Gameplay,
    Clear,
    Development,
    END
};

class Game : public Craft::Engine
{
public:
    Game();
    ~Game() override = default;

    void ChangeLevel(State state);

private:
    State _state = State::Title;
    std::vector<std::shared_ptr<Craft::Level>> _levels{};
};

