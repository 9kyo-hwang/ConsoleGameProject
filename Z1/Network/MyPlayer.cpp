#include "pch.h"
#include "MyPlayer.h"
#include <Game/Game.h>
#include <Core/Input.h>

using namespace Craft;
using namespace Z1::Protocol;

MyPlayer::MyPlayer(Vector2 position, std::uint32_t playerId)
    : Super(position, playerId)
{
}

void MyPlayer::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    Game& game = dynamic_cast<Game&>(Engine::Get());
    if (!game.IsServerConnected() || IsDead())
    {
        return;
    }

    const MoveDirection dir = InputToMoveDir();
    const std::uint8_t actionFlags = Input::Get().GetKeyDown('A') ? InputActionAttack : 0;

    if (dir == _lastSentMoveDir && actionFlags == 0)
    {
        return;
    }

    // 현재 자신의 스프라이트 생성 및 사운드 재생은 actionFlags 함께 검사함으로서 시각화 가능
    // 다만 이 방식은 다른 NetworkPlayer들에게 

    if (game.SendNetworkInput(dir, actionFlags))
    {
        _lastSentMoveDir = dir;
    }
}

Z1::Protocol::MoveDirection MyPlayer::InputToMoveDir() const
{
    if (Input::Get().GetKey(VK_UP))
    {
        return MoveDirection::Up;
    }
    else if (Input::Get().GetKey(VK_DOWN))
    {
        return MoveDirection::Down;
    }
    else if (Input::Get().GetKey(VK_LEFT))
    {
        return MoveDirection::Left;
    }
    else if (Input::Get().GetKey(VK_RIGHT))
    {
        return MoveDirection::Right;
    }

    return MoveDirection::None;
}
