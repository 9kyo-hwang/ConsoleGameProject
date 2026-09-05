#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

class PlayerGun;
class PlayerEngineEffect;

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
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void OnCollision(const std::shared_ptr<Actor>& other) override;
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

    std::vector<std::shared_ptr<PlayerGun>> _guns;  // 총구 위치 2개 사용
    std::shared_ptr<PlayerEngineEffect> _engineEffect;
};

