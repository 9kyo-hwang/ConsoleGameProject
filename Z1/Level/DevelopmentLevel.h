#pragma once
#include <Level/Level.h>
#include <World/OverworldMapLoader.h>

class CollisionTestActor;

class DevelopmentLevel : public Craft::Level
{
public:
    DevelopmentLevel();

    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;

private:
    void InitializeMapTest();
    void BuildRoomSprite();
    bool CanMove(const Craft::Vector2& candidate) const;

private:
    std::shared_ptr<CollisionTestActor> _testA;
    std::shared_ptr<CollisionTestActor> _testB;

    OverworldMapLoader _loader;
    std::optional<RoomDefinition> _room;

    std::string _mapStatus = "NOT TESTED";
    bool _attempted = false;

    TileCatalog _tileCatalog;
    std::shared_ptr<const Craft::Sprite> _roomSprite;
};

