#pragma once
#include <Level/Level.h>

namespace Craft
{
    class Actor;
    class Sprite;
}

class GameplayLevel : public Craft::Level
{
public:
    GameplayLevel();
    ~GameplayLevel() override = default;

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;

private:
    std::shared_ptr<Craft::Actor> _spriteActor;
    std::shared_ptr<const Craft::Sprite> _testSprite;
};

