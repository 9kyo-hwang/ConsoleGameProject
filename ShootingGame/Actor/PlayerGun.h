#pragma once
#include <Actor/Actor.h>

// Player에 붙어서 총알 발사 위치를 제공하는 액터
class PlayerGun : public Craft::Actor
{
    TYPE_DECLARATIONS(PlayerGun, Craft::Actor)

public:
    // 총알이 발사될 로컬 위치
    PlayerGun(const Craft::Vector2& localPosition);
    ~PlayerGun() override = default;

    // 총알이 생성될 '월드' 좌표 반환
    Craft::Vector2 GetFirePosition() const;
};

