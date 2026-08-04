#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

class Player : public Craft::Actor
{
    enum class FireMode
    {
        OneShot = 0,
        Repeat = 1
    };

    TYPE_DECLARATIONS(Player, Craft::Actor)

public:
    Player();

private:
    void Tick(float deltaTime) override;
    void Move(float direction, float deltaTime);
    void Fire();            // 단사
    void FireInterval();    // 연사

    inline bool CanFire() const { return _timer.IsTimeout(); }

private:
    float _posX = 0.f;
    float _moveSpeed = 70.f;
    FireMode _fireMode = FireMode::OneShot;
    Timer _timer;               // 연사 간격 타이머
    float _fireInterval = 0.2f; // 연사 간격(초)
};

