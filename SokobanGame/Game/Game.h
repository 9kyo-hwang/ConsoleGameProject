#pragma once

#include <Engine/Engine.h>
#include <vector>
#include <memory>

class Craft::Level;

enum class State
{
    Gameplay = 0,
    Menu = 1,
    END
};

class Game : public Craft::Engine
{
public:
    Game();
    ~Game() override = default;

    void ToggleMenu();

private:
    std::vector<std::shared_ptr<Craft::Level>> _levels{};
    State _state = State::Gameplay;
};
