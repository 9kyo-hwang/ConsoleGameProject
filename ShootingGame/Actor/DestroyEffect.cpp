#include "pch.h"
#include "DestroyEffect.h"
#include <Engine/Engine.h>
#include <Component/SpriteRendererComponent.h>

using namespace Craft;
using EffectFrame = DestroyEffect::EffectFrame;

static const std::vector<EffectFrame> effects =
{
    {"  @  ", 0.02f, Color::Red},
    {" @@  ", 0.03f, Color::Blue},
    {" @@@ ", 0.05f, Color::Green},
    {"@@@@ ", 0.08f, Color::Blue},
    {"@@@@@", 0.13f, Color::Red}
};

DestroyEffect::DestroyEffect(const Craft::Vector2& effectPosition)
    : Super(effectPosition)
{
    _renderer = AddComponent<SpriteRendererComponent>(effects[0].frame, effects[0].color, 7);

    // 이펙트 재생 위치(x) 보정
    const int frameLength = (int)effects[0].frame.size();
    
    int posX = effectPosition.x;
    if (posX < 0)  // 화면 왼쪽 밖으로 나가있다면
    {
        posX += frameLength;
    }
    else if (posX + frameLength > Engine::Get().GetWidth())  // 화면 오른쪽 밖으로 나가있다면
    {
        posX -= frameLength;
    }

    SetPosition(Vector2(posX, GetPosition().y));

    // 재생 시간 타이머 등 설정
    _timer.SetTargetTime(effects[0].playtime);
    _nextIndex = 1;
}

void DestroyEffect::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
    _timer.Tick(deltaTime);

    if (!_timer.IsTimeout())
    {
        return;
    }

    if (_nextIndex >= effects.size())
    {
        Destroy();
        return;
    }

    // 다음 프레임으로 교체: 타이머 재설정, 이미지 및 컬러 교체
    _timer.Reset();

    auto& nextEffect = effects[_nextIndex++];
    _timer.SetTargetTime(nextEffect.playtime);
    if (_renderer)
    {
        _renderer->SetImage(nextEffect.frame);
        _renderer->SetColor(nextEffect.color);
    }
}
