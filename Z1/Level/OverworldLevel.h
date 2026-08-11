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
    void BuildRoomSprite();
    bool CanMove(const Craft::Vector2& candidate) const;

private:
    std::shared_ptr<Player> _player;

    OverworldMapLoader _loader;
    bool _loaded = false;
    std::optional<RoomDefinition> _room;
    TileCatalog _tileCatalog;
    std::shared_ptr<const Craft::Sprite> _roomSprite;
};

