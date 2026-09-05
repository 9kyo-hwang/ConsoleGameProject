#pragma once
#include <Actor/Actor.h>

class PlayerBullet : public Craft::Actor
{
    TYPE_DECLARATIONS(PlayerBullet, Craft::Actor)

public:
    PlayerBullet(const Craft::Vector2& start);  // 플레이어 위치에서 시작
    ~PlayerBullet() override;

    void Tick(float deltaTime) override;    // 총알 이동 처리를 위함

private:
    float _moveSpeed = 30.f;

    float _posY = 0.f;  // x는 플레이어 위치 기준으로 고정, 위로만 이동
};

