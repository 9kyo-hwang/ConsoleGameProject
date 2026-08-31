#pragma once
#include <Level/Level.h>
#include <World/OverworldMap.h>
#include <unordered_map>
#include <Level/Room.h>
#include <Level/EnemySpawner.h>
#include <Actor/Octorok.h>
#include <Util/Timer.h>
#include <optional>

#include <Z1Shared/Protocol.h>

namespace Craft
{
    class Sprite;
}

class Pawn;
class Player;
class Enemy;
class Projectile;
struct ProjectileSpec;

class NetworkPlayer;
class NetworkEnemy;
class MyPlayer;
class Game;

enum class EntranceType
{
    SwordCave,
    Dungeon1
};

class OverworldLevel : public Craft::Level
{
public:
    OverworldLevel();

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

    bool CanProjectileOccupy(Craft::Vector2 destination, const Projectile& projectile);
    std::shared_ptr<Projectile> SpawnProjectile(Craft::Vector2 position, const ProjectileSpec& spec, const std::shared_ptr<Pawn>& instigator);
    std::shared_ptr<Enemy> SpawnEnemy(const EnemySpawnData& spawn);

    // Network
    void ApplyLatestNetworkSnapshot(Game& game);
    void EnsureOfflinePlayers(Game& game);  // 서버 연결 끊기면 기존 싱글 플레이 유지를 위한 역할
    void ClearNetworkActors();

private:
    bool LoadMap();
    bool TryChangeRoom(RoomCoordinate room);
    void BuildRoomSprite();
    
    RoomCoordinate GetRoomCoordinate(const Craft::Vector2& mapCellPosition) const;
    RoomCoordinate GetRoomCoordinateAtLeadingEdge(Craft::Vector2 destination, const Pawn& pawn, Craft::Vector2 direction) const;

    Craft::Vector2 GetRoomCellOrigin(RoomCoordinate room) const;
    void SnapPlayerIntoRoom(RoomCoordinate room, Craft::Vector2 direction);

    bool CanMoveTo(Craft::Vector2 destination, const Pawn& mover, bool allowContactEscape = false);
    bool UpdatePawnKnockback(Pawn& pawn, float deltaTime);

    void SpawnRoomEnemies();
    void DestroyRoomEnemies();

    void UpdatePlayerMovement(float deltaTime, const Craft::Vector2& delta);
    void UpdateEnemyMovement(float deltaTime);

    void DestroyRoomProjectiles();

    bool IsInsideCurrentRoom(Craft::Vector2 boxPosition, Craft::Vector2 boxSize);

    void TakeContactDamageToPlayer();

    std::optional<EntranceType> ResolveEntrance(Craft::Vector2 destination, const Pawn& mover);
    bool TryEnterEntrance(Craft::Vector2 destination);

private:    // Network
    void ApplyNetworkRoom(const Z1::Protocol::SnapshotPlayerState& state);  // 기존 TryChangeRoom 대신 네트워크 전용

private:
    inline static constexpr RoomCoordinate StartRoom{ 7, 7 };

    std::shared_ptr<Player> _player;

    OverworldMap _map;
    std::shared_ptr<const Craft::Sprite> _roomSprite;
    RoomCoordinate _currentRoom{ 7, 7 };

    bool _loaded = false;
    bool _bgmStarted = false;
    bool _playerStateLoaded = false;
    bool _gameOverPending = false;
    Timer _gameOverTimer{ 2.7f };

    EnemySpawner _enemySpawner;
    std::vector<std::shared_ptr<Enemy>> _roomEnemies;   // 현재 룸에 생성된 적 별도 보관
    std::vector<std::shared_ptr<Projectile>> _roomProjectiles;
    uint32_t _worldSeed = 12345u;

    /************
    * Network
    ************/

    // 서버는 마지막으로 기록한 input 방향을 매 tick 적용
    // 따라서 아래의 경우에만 패킷 전송
    // - 이동 시작: None -> up
    // - 방향 변경: Up - Left
    // - 키 놓아 이동 정지: Left -> None
    // - 공격 키 눌림

    std::shared_ptr<MyPlayer> _myPlayer;
    std::unordered_map<std::uint32_t, std::shared_ptr<NetworkPlayer>> _networkPlayers;
    std::unordered_map<std::uint32_t, std::shared_ptr<NetworkEnemy>> _networkEnemies;
    std::optional<std::uint32_t> _lastAppliedServerTick;
};
