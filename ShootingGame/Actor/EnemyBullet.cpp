#include "pch.h"
#include "EnemyBullet.h"
#include <Engine/Engine.h>
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

EnemyBullet::EnemyBullet(const Craft::Vector2& startPosition, float moveSpeed)
    : Super(startPosition)
    , _moveSpeed(moveSpeed)
    , _posY((float)startPosition.y)
{
    auto sprite = Sprite::Create("#", Color::Red);
    AddComponent<SpriteRendererComponent>(sprite, 4);
    AddComponent<BoxComponent>(sprite->GetSize());
}

void EnemyBullet::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _posY += _moveSpeed * deltaTime;
    if (_posY >= Engine::Get().GetHeight() - 1)  // 플레이어 생성 위치 기준에 맞추도록
    {
        Destroy();
        return;
    }

    Vector2 newPosition = GetPosition();
    newPosition.y = (int32)_posY;
    SetPosition(newPosition);
}
