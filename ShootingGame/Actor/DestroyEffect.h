#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

class DestroyEffect : public Craft::Actor
{
    TYPE_DECLARATIONS(DestroyEffect, Craft::Actor)

    struct EffectFrame
    {
        std::string frame{};  // 화면에 보여질 이미지
        float playtime = 0.f;   // 이펙트 재생 시간
        Craft::Color color = Craft::Color::White;

        EffectFrame(const std::string& frame, float playtime, Craft::Color color)
            : frame(frame)
            , playtime(playtime)
            , color(color)
        {

        }

        ~EffectFrame() = default;
    };

public:
    // 폭발 위치
    DestroyEffect(const Craft::Vector2& effectPosition);
    ~DestroyEffect() = default;

private:
    // 재생 시간이 있으므로
    void Tick(float deltaTime) override;

private:
    int _nextIndex;    // 다음에 표시될 이펙트 인덱스(이미 0번 세팅이 진행돼서)
    Timer _timer;
};

