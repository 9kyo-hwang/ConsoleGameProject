#pragma once

#include <Level/Level.h>
#include <Level/Room.h>
#include <World/DungeonMap.h>
#include <Util/Timer.h>

namespace Craft
{
    class Sprite;
}

class Pawn;
class Player;
class Enemy;
class Projectile;
struct ProjectileSpec;

class DungeonLevel : public Craft::Level
{
public:
    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

    bool CanProjectileOccupy(
        Craft::Vector2 destination,
        const Projectile& projectile
    );

    std::shared_ptr<Projectile> SpawnProjectile(
        Craft::Vector2 position,
        const ProjectileSpec& spec,
        const std::shared_ptr<Pawn>& instigator
    );

private:
    bool LoadMap();
    bool TryChangeRoom(RoomCoordinate room);
    void BuildRoomSprite();

    void SpawnRoomEnemies();
    std::shared_ptr<Enemy> SpawnEnemy(Craft::Vector2 position);
    void SpawnBoss();

    void DestroyRoomEnemies();
    void DestroyRoomProjectiles();

    void UpdatePlayerMovement(
        float deltaTime,
        const Craft::Vector2& delta
    );

    bool TryExitDungeon(
        const Craft::Vector2& destination,
        const Craft::Vector2& moveDelta
    );

    void UpdateEnemyMovement(float deltaTime);

    bool UpdatePawnKnockback(
        Pawn& pawn,
        float deltaTime
    );

    bool CanMoveTo(
        const Craft::Vector2& destination,
        const Pawn& mover
    );

    bool IsInsideCurrentRoom(
        Craft::Vector2 boxPosition,
        Craft::Vector2 boxSize
    ) const;

    RoomCoordinate GetRoomCoordinate(
        const Craft::Vector2& worldPosition
    ) const;

    RoomCoordinate GetRoomCoordinateAtLeadingEdge(
        const Craft::Vector2& destination,
        const Pawn& pawn,
        const Craft::Vector2& direction
    ) const;

    Craft::Vector2 GetRoomWorldOrigin(
        RoomCoordinate room
    ) const;

    void SnapPlayerIntoRoom(
        RoomCoordinate room,
        const Craft::Vector2& direction
    );

    void TakeContactDamageToPlayer();
    void UpdateBossState();
    void TryCollectItems();

    bool IsPlayerOverlappingTile(
        const Craft::Vector2& tile
    ) const;

private:
    static constexpr int BossRoomIndex = 3;
    static constexpr int RandomEnemyRoomLastIndex = 2;

    static constexpr int EnemyCount = 6;
    static constexpr int MaxSpawnAttempts = 30;

    // Level1.txt에는 Player 마커가 없으므로 입구 바로 위에서 시작한다.
    static constexpr int PlayerSpawnTileX = 7;
    static constexpr int PlayerSpawnTileY = 8;

    bool _loaded = false;
    bool _bgmStarted = false;
    bool _needsPlayerSync = true;
    bool _gameOverPending = false;
    Timer _gameOverTimer{ 0.4f };
    bool _clearPending = false;
    Timer _clearTimer{ 6.0f };

    DungeonMap _map;
    std::shared_ptr<const Craft::Sprite> _roomSprite;

    RoomCoordinate _currentRoom{ 0, 0 };

    std::shared_ptr<Player> _player;

    Craft::Vector2 _bossTile = Craft::Vector2::Zero;
    Craft::Vector2 _heartTile = Craft::Vector2::Zero;
    Craft::Vector2 _triforceTile = Craft::Vector2::Zero;

    std::shared_ptr<Enemy> _boss;
    bool _bossDefeated = false;
    bool _heartCollected = false;
    bool _triforceCollected = false;

    std::vector<std::shared_ptr<Enemy>> _roomEnemies;
    std::vector<std::shared_ptr<Projectile>> _roomProjectiles;

    uint32_t _worldSeed = 12345u;
};
