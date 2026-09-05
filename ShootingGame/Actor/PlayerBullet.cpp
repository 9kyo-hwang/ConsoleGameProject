#include "pch.h"
#include "PlayerBullet.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

PlayerBullet::PlayerBullet(const Craft::Vector2& start)
    : Super(start)
    , _posY((float)start.y)
{
    auto sprite = Sprite::Create("@", Color::Blue);
    AddComponent<SpriteRendererComponent>(sprite, 4);
    AddComponent<BoxComponent>(sprite->GetSize());
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

    Vector2 newPosition = GetPosition();
    newPosition.y = (int32)_posY;
    SetPosition(newPosition);
}
