#include <pch.h>
#include "Player.h"
#include <Engine/Engine.h>
#include <Core/Input.h>

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

    Vector2 newPosition = position;
    newPosition.x = (int)_posX;
    SetPosition(newPosition);
}
