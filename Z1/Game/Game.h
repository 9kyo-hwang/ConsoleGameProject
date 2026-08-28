#pragma once

#include <Engine/Engine.h>
#include <vector>
#include <Network/NetworkClient.h>
#include <optional>

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
    ~Game() override = default; // Game -> Engine 순으로 소멸, Game 소멸 시 NetworkClient 소멸되며 자연스레 Stop

    void StartNewGame();
    void ChangeLevel(State state);

    void ResetPlayerState();
    void SavePlayerState(const Player& player);
    void LoadPlayerState(Player& player) const;

public: // Network
    bool ConnectToServer();
    void PumpNetwork();
    inline bool IsServerConnected() const noexcept { return _network.IsConnected(); }

    std::optional<std::uint32_t> GetLocalPlayerId() const;
    const std::optional<WorldSnapshot>& GetLatestSnapshot() const;

private:
    struct PlayerState
    {
        int hp = PlayerMaxHp;
        bool hasSword = false;
    };

    State _state = State::Title;
    std::vector<std::shared_ptr<Craft::Level>> _levels{};
    PlayerState _playerState{};

private:    // Network
    NetworkClient _network;
    std::optional<std::uint32_t> _localPlayerId;
    std::optional<WorldSnapshot> _latestSnapshot;
};

