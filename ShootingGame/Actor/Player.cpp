#include "pch.h"
#include "Player.h"
#include <Engine/Engine.h>
#include <Core/Input.h>
#include <Level/Level.h>
#include <Actor/PlayerBullet.h>
#include <Actor/EnemyBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/GameManager.h>

using namespace Craft;

Player::Player()
    : Super("<=A=>", Vector2::Zero, Color::Green)
{
    // 콘솔 가운데에 위치 + 플레이어 가로 길이 보정
    int x = (Engine::Get().GetWidth() / 2) - (width / 2);
    
    // 콘솔 최하단에서 + 2
    int y = Engine::Get().GetHeight() - 2;

    SetPosition(Vector2(x, y));

    _posX = (float)x;

    _timer.SetTargetTime(_fireInterval);
}

void Player::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    if (Input::Get().GetKeyDown(VK_ESCAPE))
    {
        Engine::Get().Quit();
        return;
    }

    float direction = 0.f;
    if (Input::Get().GetKey(VK_RIGHT))
    {
        direction = 1.f;
    }
    if (Input::Get().GetKey(VK_LEFT))
    {
        direction = -1.f;
    }

    Move(direction, deltaTime);

    _timer.Tick(deltaTime);
    if (_fireMode == FireMode::OneShot && Input::Get().GetKeyDown(VK_SPACE))
    {
        Fire();
    }
    else if (_fireMode == FireMode::Repeat && Input::Get().GetKey(VK_SPACE))
    {
        FireInterval();
    }

    if (Input::Get().GetKeyDown('R'))
    {
        _fireMode = (FireMode)(1 - (int32)_fireMode);
    }
}

void Player::OnCollision(const std::shared_ptr<Actor>& other)
{
    Super::OnCollision(other);

    if (other->IsA<EnemyBullet>())
    {
        Engine::Get().PlayOneShot("Explosion.wav"); // 폭발 사운드
        
        Destroy();
        other->Destroy();

        // TODO: GameManager에게 플레이어 죽음 알림(게임 종료 등)
        if (auto level = GetOwner())
        {
            // 사망 이펙트 발생 X: 게임오버 시 레벨 단에서 [!DEAD!] 출력
            if (auto gameManager = level->FindActor<GameManager>())
            {
                gameManager->SetPlayerDead(position);
            }
        }

        return;
    }
}

void Player::Move(float direction, float deltaTime)
{
    _posX += direction * _moveSpeed * deltaTime;

    // 플레이어가 화면 밖으로 안벗어나도록
    if (_posX < 0.f)
    {
        _posX = 0.f;
    }
    else if (_posX + width >= Engine::Get().GetWidth())
    {
        _posX = (float)Engine::Get().GetWidth() - width;
    }

    position.x = (int)_posX;
}

void Player::Fire()
{
    // Bullet 생성 -> Level도 필요
    Vector2 bulletPosition = position;
    bulletPosition.x += width / 2;  // 좌표 기준은 왼쪽 끝, 총알은 가운데로
    bulletPosition.y -= 1;  // 자기 자신 위치에 + 1
    GetOwner()->SpawnActor<PlayerBullet>(bulletPosition);

    Engine::Get().PlayOneShot("Retro_Laser_Shoot.wav");
}

void Player::FireInterval()
{
    if (!CanFire()) return;

    _timer.Reset();
    Fire();
}
