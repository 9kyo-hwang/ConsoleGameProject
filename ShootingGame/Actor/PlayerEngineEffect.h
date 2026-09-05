#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

namespace Craft
{
    class SpriteRendererComponent;
}

// 플레이어 아래에 붙어 추진 효과를 보여주는 역할
class PlayerEngineEffect : public Craft::Actor
{
    TYPE_DECLARATIONS(PlayerEngineEffect, Craft::Actor)

public:
    // 엔진 이펙트 로컬 좌표
    PlayerEngineEffect(const Craft::Vector2& localPosition);
    ~PlayerEngineEffect() override = default;

private:
    void Tick(float deltaTime) override;    // 일종의 애니메이션 이펙트 필요

private:
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;  // 애니메이션 프레임 교체용
    float _elapsedTime = 0.f;   // 애니메이션 처리를 위한 시간
    float _frameTime = 0.08f;   // 애니메이션 한 프레임 재생 시간(초)
    int _frameIndex = 0;        // 현재 재생중인 애니메이션 프레임 인덱스
};

