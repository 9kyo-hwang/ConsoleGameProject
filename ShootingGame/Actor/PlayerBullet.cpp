#include "pch.h"
#include "PlayerBullet.h"

using namespace Craft;

PlayerBullet::PlayerBullet(const Craft::Vector2& start)
    : Super("@", start, Color::Blue)
    , _posY((float)start.y)
{
}

PlayerBullet::~PlayerBullet()
{
}

void PlayerBullet::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _posY -= _moveSpeed * deltaTime;
    if (_posY < 0.f)
    {
        Destroy();
        return;
    }

    position.y = (int32)_posY;
}
