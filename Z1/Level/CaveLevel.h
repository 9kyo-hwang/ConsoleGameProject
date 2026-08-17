#pragma once
#include <Level/Level.h>
#include <array>

namespace Craft
{
    class Sprite;
}

class CaveLevel : public Craft::Level
{
public:
    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

private:
    bool LoadMap();
    void BuildRoomSprite();

    bool CanMoveTo(Craft::Vector2 position) const;
    bool IsOnTile(Craft::Vector2 tile) const;

    bool UpdatePlayerMovement(float deltaTime);
    bool TryCollectSword();
    bool TryExitCave(Craft::Vector2 destination, Craft::Vector2 moveDelta);

private:
    static constexpr int RoomWidth = 16;
    static constexpr int RoomHeight = 11;
    static constexpr int TilePixelWidth = 10;
    static constexpr int TilePixelHeight = 5;

    using RoomRow = std::array<char, RoomWidth>;
    using RoomGrid = std::array<RoomRow, RoomHeight>;

private:
    RoomGrid _grid{};

    Craft::Vector2 _playerPosition = Craft::Vector2::Zero;
    Craft::Vector2 _swordPosition = Craft::Vector2::Zero;
    Craft::Vector2 _exitPosition = Craft::Vector2::Zero;

    std::shared_ptr<const Craft::Sprite> _roomSprite;
    std::shared_ptr<class Player> _player;

    bool _loaded = false;
    bool _swordCollected = false;
    bool _needsPlayerSync = true;
};

