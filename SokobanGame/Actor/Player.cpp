#include <pch.h>
#include "Player.h"
#include <Core/Input.h>
#include <Interface/ICanPlayerMove.h>
#include <Level/Level.h>

using namespace Craft;

Player::Player(const Craft::Vector2& position)
    : Super("P", position, Color::Green)
{
    sortingOrder = 5;
}

void Player::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // ESC키 종료
    if(Input::Get().GetKeyDown(VK_ESCAPE))
    {
        QuitGame();
    }

    /*
    * 이제부터 플레이어가 이동하려면 "이동하고자 하는 칸"을 확인해야 함
    * 이동 가능한지 여부, 박스인 경우 그 다음 칸까지.
    * Level이 액터 목록을 들고 있으나, Player가 Level에 질의하는 건 불필요한 커플링
    * 중간 계층을 추가해 그 부분으로 소통하도록
    */

    std::shared_ptr<ICanPlayerMove> canPlayerMove = std::dynamic_pointer_cast<ICanPlayerMove>(GetOwner());
    if (!canPlayerMove)
    {
        return;
    }

    Vector2 newPosition = position;

    if (Input::Get().GetKeyDown(VK_LEFT))
    {
        newPosition.x = position.x - 1;
    }

    if (Input::Get().GetKeyDown(VK_RIGHT))
    {
        newPosition.x = position.x + 1;
    }

    if (Input::Get().GetKeyDown(VK_UP))
    {
        newPosition.y = position.y - 1;
    }

    if (Input::Get().GetKeyDown(VK_DOWN))
    {
        newPosition.y = position.y + 1;
    }

    if (canPlayerMove->CanMoveTo(position, newPosition))
    {
        SetPosition(newPosition);
    }
}
