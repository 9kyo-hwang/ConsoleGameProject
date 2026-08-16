#pragma once
#include <Level/Level.h>
#include <World/OverworldMap.h>
#include <unordered_map>
#include <Level/Room.h>
#include <Level/EnemySpawner.h>
#include <Actor/Octorok.h>

namespace Craft
{
    class Sprite;
}

using TileSpriteMap = std::unordered_map<TileId, std::shared_ptr<const Craft::Sprite>>;

class Pawn;
class Player;
class Enemy;
class Projectile;
struct ProjectileSpec;

enum class EntranceType
{
    SwordCave,
    // Dungeon1, ...
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

private:
    bool LoadMap();
    bool TryChangeRoom(RoomCoordinate room);
    void BuildRoomSprite();
    
    RoomCoordinate GetRoomCoordinate(const Craft::Vector2& mapPosition) const;
    Craft::Vector2 GetRoomWorldOrigin(RoomCoordinate room) const;

    bool CanMoveTo(const Craft::Vector2& destination, const Pawn& mover);
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

private:
    inline static constexpr RoomCoordinate StartRoom{ 7, 7 };

    std::shared_ptr<Player> _player;

    OverworldMap _map;
    TileSpriteMap _tileSprites;

    std::shared_ptr<const Craft::Sprite> _roomSprite;
    RoomCoordinate _currentRoom{ 7, 7 };

    bool _loaded = false;
    bool _bgmStarted = false;

    EnemySpawner _enemySpawner;
    std::vector<std::shared_ptr<Enemy>> _roomEnemies;   // 현재 룸에 생성된 적 별도 보관
    uint32_t _worldSeed = 12345u;

    std::vector<std::shared_ptr<Projectile>> _roomProjectiles;
};

