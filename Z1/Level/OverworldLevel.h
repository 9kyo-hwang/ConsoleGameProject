#pragma once
#include <Level/Level.h>
#include <World/OverworldMap.h>
#include <unordered_map>

namespace Craft
{
    class Sprite;
}

using TileSpriteMap = std::unordered_map<TileId, std::shared_ptr<const Craft::Sprite>>;

struct RoomCoordinate
{
    int x = 0;
    int y = 0;

    bool operator==(const RoomCoordinate&) const = default;
};

class Player;
class Enemy;
class OverworldLevel : public Craft::Level
{
public:
    OverworldLevel();

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

private:
    bool LoadMap();
    void ChangeRoom(RoomCoordinate room);
    void BuildRoomSprite();
    
    RoomCoordinate GetRoomCoordinate(const Craft::Vector2& mapPosition) const;
    Craft::Vector2 GetRoomWorldOrigin(RoomCoordinate room) const;

    bool CanMove(const Craft::Vector2& candidate) const;

    uint32_t MakeRoomSeed(RoomCoordinate room) const;
    void SpawnRoomEnemies();
    void DestroyRoomEnemies();

private:
    inline static constexpr RoomCoordinate StartRoom{ 7, 7 };

    std::shared_ptr<Player> _player;

    OverworldMap _map;
    TileSpriteMap _tileSprites;

    std::shared_ptr<const Craft::Sprite> _roomSprite;
    RoomCoordinate _currentRoom{ 7, 7 };

    bool _loaded = false;
    bool _bgmStarted = false;

    std::vector<std::shared_ptr<Enemy>> _roomEnemies;   // 현재 룸에 생성된 적 별도 보관
    uint32_t _worldSeed = 12345u;
};

