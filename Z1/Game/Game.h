#pragma once

#include <Engine/Engine.h>
#include <vector>

namespace Craft
{
    class Level;
}

class Player;

enum class State
{
    Title,
    Overworld,
    SwordCave,
    Dungeon1,
    Clear,
    GameOver,
    Development,
    END
};

class Game : public Craft::Engine
{
public:
    inline static constexpr int PlayerMaxHp = 20;

    Game();
    ~Game() override = default;

    void StartNewGame();
    void ChangeLevel(State state);

    void ResetPlayerState();
    void SavePlayerState(const Player& player);
    void LoadPlayerState(Player& player) const;

private:
    struct PlayerState
    {
        int hp = PlayerMaxHp;
        bool hasSword = false;
    };

    State _state = State::Title;
    std::vector<std::shared_ptr<Craft::Level>> _levels{};
    PlayerState _playerState{};
};

