#pragma once

#include <Actor/Actor.h>
#include <Math/Vector2.h>
#include <Math/Color.h>
#include <Util/Timer.h>

namespace Craft
{
    class Sprite;
}

enum class CombatEffectType
{
    SwordSlash,
    Hit,
    Death
};

std::shared_ptr<const Craft::Sprite> CreateCombatEffectSprite(
    CombatEffectType type,
    Craft::Vector2 direction = Craft::Vector2::Zero,
    Craft::Color color = Craft::Color::White
);

class CombatEffect : public Craft::Actor
{
    TYPE_DECLARATIONS(CombatEffect, Craft::Actor)

public:
    CombatEffect(
        Craft::Vector2 position,
        std::shared_ptr<const Craft::Sprite> sprite,
        float duration,
        int sortingOrder = 20
    );

    void Tick(float deltaTime) override;

private:
    Timer _timer;
};
