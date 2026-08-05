#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

class EnemySpawner : public Craft::Actor
{
    TYPE_DECLARATIONS(EnemySpawner, Craft::Actor)

public:
    EnemySpawner();
    ~EnemySpawner() override = default;

private:
    void Tick(float deltaTime) override;

    void Spawn();   // timer를 이용한 주기적 적 생성

private:
    Timer _timer;
};

