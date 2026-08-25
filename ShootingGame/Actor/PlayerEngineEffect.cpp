#include "pch.h"
#include "PlayerEngineEffect.h"
#include <Component/SpriteRendererComponent.h>

// 여기 소스파일에서만 사용(static)
static const std::vector<const char*> effects = 
{
    "  *  ",
    " *** ",
    "*****"
};

using namespace Craft;

PlayerEngineEffect::PlayerEngineEffect(const Craft::Vector2& localPosition)
    : Super(localPosition)
{
    // 재사용을 위해 캐싱
    _renderer = AddComponent<SpriteRendererComponent>(Sprite::Create(effects[0], Color::Red), 4);
}

void PlayerEngineEffect::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _elapsedTime += deltaTime;
    if (_elapsedTime < _frameTime)
    {
        return;
    }

    // 다음 장으로 넘기기
    _elapsedTime = 0.f;
    _frameIndex = (_frameIndex + 1) % (int32)effects.size();
    if (_renderer)
    {
        _renderer->SetSprite(Sprite::Create(effects[_frameIndex]));
    }
}
