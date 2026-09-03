#pragma once

#include <Engine/Engine.h>
#include <vector>
#include <Network/NetworkClient.h>
#include <optional>
#include <unordered_map>

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
    inline bool IsServerConnected() const noexcept { return _network.IsConnected(); }
    bool ConnectToServer();
    void PumpNetwork();

    inline std::optional<std::uint32_t> GetLocalPlayerId() const { return _localPlayerId; }
    inline const std::optional<Z1::Protocol::WorldSnapshot>& GetLatestSnapshot() const { return _latestSnapshot; }

    // Overworld Level에서 Game-Network에 데이터 밀어넣기 위한 래퍼
    inline bool SendNetworkInput(Z1::Protocol::MoveDirection direction, std::uint8_t actionFlags)
    {
        return _network.QueueInput(direction, actionFlags);
    }

    inline std::vector<Z1::Protocol::CombatEvent> ConsumeCombatEvents()
    {
        return std::exchange(_pendingCombatEvents, std::vector<Z1::Protocol::CombatEvent>());
    }

    inline const std::unordered_map<std::uint32_t, Z1::Protocol::EnemyPathDebug>& GetLatestEnemyPathDebugs()
    {
        return _latestEnemyPaths;
    }

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
    std::optional<Z1::Protocol::WorldSnapshot> _latestSnapshot;

    // 이벤트는 모두 들고 있고, 모두 소비해야 함
    std::vector< Z1::Protocol::CombatEvent> _pendingCombatEvents;

    // EnemyPathDebug는 id 별로 최신값 1개씩만 가지고 있다.
    std::unordered_map<std::uint32_t, Z1::Protocol::EnemyPathDebug> _latestEnemyPaths;
};

