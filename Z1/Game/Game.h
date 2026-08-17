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
    Overworld,
    SwordCave,
    Dungeon1,
    Gameplay,
    Clear,
    GameOver,
    Development,
    END
};

class Game : public Craft::Engine
{
public:
    Game();
    ~Game() override = default;

    void StartNewGame();
    void ChangeLevel(State state);

    inline bool HasSword() const { return _hasSword; }
    void SetHasSword(bool value) { _hasSword = value; }

private:
    State _state = State::Title;
    std::vector<std::shared_ptr<Craft::Level>> _levels{};
    bool _hasSword = false;
};

