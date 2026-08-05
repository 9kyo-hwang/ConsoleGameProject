#include "pch.h"
#include "EnemyBullet.h"
#include <Engine/Engine.h>

using namespace Craft;

EnemyBullet::EnemyBullet(const Craft::Vector2& startPosition, float moveSpeed)
    : Super("#", startPosition, Color::Red)
    , _moveSpeed(moveSpeed)
    , _posY((float)startPosition.y)
{

}

void EnemyBullet::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _posY += _moveSpeed * deltaTime;
    if (_posY >= Engine::Get().GetHeight() - 2)  // 플레이어 생성 위치 기준에 맞추도록
    {
        Destroy();
        return;
    }

    position.y = (int32)_posY;
}
