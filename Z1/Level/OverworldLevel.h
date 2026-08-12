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

private:
    std::shared_ptr<Player> _player;

    OverworldMap _map;
    TileSpriteMap _tileSprites;

    std::shared_ptr<const Craft::Sprite> _roomSprite;
    RoomCoordinate _currentRoom{ 7, 7 };

    bool _loaded = false;
    bool _bgmStarted = false;
};

