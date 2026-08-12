#pragma once
#include <Level/Level.h>
#include <World/OverworldMapLoader.h>

class Player;

class OverworldLevel : public Craft::Level
{
public:
    OverworldLevel();

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;

private:
    void LoadMap();
    bool LoadRoom(RoomCoordinate room, std::string& error);
    void BuildRoomSprite();
    RoomCoordinate GetRoomCoordinate(const Craft::Vector2& mapPosition) const;
    Craft::Vector2 GetRoomWorldOrigin(RoomCoordinate room) const;
    bool CanMove(const Craft::Vector2& candidate) const;

private:
    std::shared_ptr<Player> _player;

    OverworldMapLoader _loader;
    TileSpriteCatalog _tileSprites;

    bool _loaded = false;
    std::optional<RoomData> _roomData;
    std::shared_ptr<const Craft::Sprite> _roomSprite;
    RoomCoordinate _currentRoom{ 7, 7 };
};

