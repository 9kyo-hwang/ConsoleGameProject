#include "pch.h"
#include "Enemy.h"
#include <Math/MathUtility.h>
#include <Engine/Engine.h>
#include <Actor/EnemyBullet.h>
#include <Level/Level.h>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/GameManager.h>

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
    
    // 총알이 가운데서 발사되도록 폭 / 2 값만큼 shift, 자신의 위치 한 칸 아래에 총알 생성
    Vector2 bulletPos(position.x + (width / 2), position.y + 1);
    GetOwner()->SpawnActor<EnemyBullet>(bulletPos, FMath::RandRange(10.f, 20.f));
}

void Enemy::OnCollision(const std::shared_ptr<Actor>& other)
{
    Super::OnCollision(other);

    if (other->IsA<PlayerBullet>())
    {
        Engine::Get().PlayOneShot("Explosion.wav");

        Destroy();
        other->Destroy();

        // 적이 죽은 위치에서 이펙트 생성
        if (auto level = GetOwner())
        {
            level->SpawnActor<DestroyEffect>(position);
            if (auto gameManager = level->FindActor<GameManager>())
            {
                gameManager->SetScore(gameManager->GetScore() + 1);
            }
        }

        return;
    }
}
