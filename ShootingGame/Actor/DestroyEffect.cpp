#include "pch.h"
#include "DestroyEffect.h"
#include <Engine/Engine.h>

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
    : Super(effects[0].frame, effectPosition, effects[0].color)
{
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

    position.x = posX;

    // 재생 시간 타이머 등 설정
    _timer.SetTargetTime(effects[0].playtime);
    _currentIndex = 0;
}

void DestroyEffect::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
    _timer.Tick(deltaTime);

    if (!_timer.IsTimeout())
    {
        return;
    }

    if (_currentIndex + 1 >= effects.size())
    {
        Destroy();
        return;
    }

    // 프레임 교체: 타이머 재설정, 이미지 및 컬러 교체
    auto& effect = effects[++_currentIndex];
    _timer.Reset();
    _timer.SetTargetTime(effect.playtime);
    ChangeImage(effect.frame);
    color = effect.color;
}
