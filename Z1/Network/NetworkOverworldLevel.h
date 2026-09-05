#pragma once
#include <Level/Level.h>
#include <Z1Shared/Protocol.h>
#include <optional>
#include <unordered_map>
#include <Level/Room.h>
#include <World/OverworldMap.h>
#include <Util/Timer.h>

namespace Craft
{
    class Sprite;
    class Renderer;
}

class Game;
class MyPlayer;
class NetworkPlayer;
class NetworkEnemy;
class NetworkProjectile;

class NetworkOverworldLevel : public Craft::Level
{
public:
    NetworkOverworldLevel();

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;

public:
    void UpdateSnapshot(Game& game);
    void ApplyCombatEvent(const Z1::Protocol::CombatEvent& event);
    void Clear();

private:
    bool LoadMap();
    void TryChangeRoom(const Z1::Protocol::SnapshotPlayerState& state);
    void BuildRoomSprite();
    void DrawLatestEnemyPathDebug(Game& game, Craft::Renderer& renderer, const Craft::Vector2 roomCellOrigin);

private:
    bool _loaded = false;
    bool _bgmStarted = false;
    bool _playerStateLoaded = false;
    bool _gameOverPending = false;
    Timer _gameOverTimer{ 2.7f };

    inline static constexpr RoomCoordinate StartRoom{ 7, 7 };
    OverworldMap _map;
    std::shared_ptr<const Craft::Sprite> _roomSprite;
    RoomCoordinate _currentRoom{ 7, 7 };

    // 서버는 마지막으로 기록한 input 방향을 매 tick 적용
    // 따라서 아래의 경우에만 패킷 전송
    // - 이동 시작: None -> up
    // - 방향 변경: Up - Left
    // - 키 놓아 이동 정지: Left -> None
    // - 공격 키 눌림

    std::shared_ptr<MyPlayer> _myPlayer;
    std::unordered_map<std::uint32_t, std::shared_ptr<NetworkPlayer>> _networkPlayers;
    std::unordered_map<std::uint32_t, std::shared_ptr<NetworkEnemy>> _networkEnemies;
    std::unordered_map<std::uint32_t, std::shared_ptr<NetworkProjectile>> _networkProjectiles;
    std::optional<std::uint32_t> _lastAppliedServerTick;

    bool _showEnemyPathDebug = false;
};
