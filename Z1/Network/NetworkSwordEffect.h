#pragma once
#include <Actor/Actor.h>
#include <Z1Shared/Protocol.h>
#include <Util/Timer.h>

class NetworkSwordEffect : public Craft::Actor
{
    TYPE_DECLARATIONS(NetworkSwordEffect, Craft::Actor)

public:
    NetworkSwordEffect(Z1::Protocol::MoveDirection direction);

    void Tick(float deltaTime) override;

private:
    Timer _timer;
};

