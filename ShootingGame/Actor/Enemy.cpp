#include "pch.h"
#include "Enemy.h"
#include <Math/MathUtility.h>
#include <Engine/Engine.h>
#include <Actor/EnemyBullet.h>
#include <Level/Level.h>

using namespace Craft;

Enemy::Enemy(const std::string& image, int posY)
    : Super(image)
{
    int random = FMath::RandRange(1, 2);
    if (random == 1)
    {
        // 화면 오른쪽 끝에서 시작하고 왼쪽으로
        _dir = MoveDirection::Left;
        _posX = (float)(Engine::Get().GetWidth() - 1 - width);
    }
    else
    {
        // 화면 왼쪽 끝에서 시작하고 오른쪽으로
        _dir = MoveDirection::Right;
        _posX = 0;
    }

    position = { (int)_posX, posY };
    _timer.SetTargetTime(FMath::RandRange(1.f, 3.f));   // 발사 간격 1초 ~ 3초 사이
}

void Enemy::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _posX += _moveSpeed * deltaTime * (float)_dir;
    if (_posX + width < 0)  // 완전히 사라졌다면(왼쪽 pos + 길이)
    {
        Destroy();
        return;
    }

    if (_posX >= Engine::Get().GetWidth())  // 완전히 사라졌다면(왼쪽 pos)
    {
        Destroy();
        return;
    }

    position.x = (int)_posX;
    _timer.Tick(deltaTime);
    
    if (!_timer.IsTimeout())
    {
        return;
    }

    _timer.Reset();
    
    // 총알이 가운데서 발사되도록 폭 / 2 값만큼 shift
    Vector2 bulletPos(position.x + (width / 2), position.y);
    GetOwner()->SpawnActor<EnemyBullet>(bulletPos, FMath::RandRange(10.f, 20.f));
}
