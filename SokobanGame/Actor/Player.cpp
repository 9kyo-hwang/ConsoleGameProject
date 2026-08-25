#include <pch.h>
#include "Player.h"
#include <Core/Input.h>
#include <Interface/ICanPlayerMove.h>
#include <Level/Level.h>
#include <Game/Game.h>
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

Player::Player(const Craft::Vector2& position)
    : Super(position)
{
    auto sprite = Sprite::Create("P", Color::Green);
    AddComponent<SpriteRendererComponent>(sprite, 5);
    AddComponent<BoxComponent>(sprite->GetSize());
}

void Player::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    if(Input::Get().GetKeyDown(VK_ESCAPE))
    {
        //QuitGame();

        Game& game = dynamic_cast<Game&>(Craft::Engine::Get());
        game.ToggleMenu();
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

    // Actor의 position은 안쓰도록 수정
    Vector2 from = GetPosition();   // 소코반에선 local/world 크게 의미 없음
    Vector2 to = from;

    if (Input::Get().GetKeyDown(VK_LEFT))
    {
        to.x -= 1;
    }

    if (Input::Get().GetKeyDown(VK_RIGHT))
    {
        to.x += 1;
    }

    if (Input::Get().GetKeyDown(VK_UP))
    {
        to.y -= 1;
    }

    if (Input::Get().GetKeyDown(VK_DOWN))
    {
        to.y += 1;
    }

    if (canPlayerMove->CanMoveTo(from, to))
    {
        SetPosition(to);    // transform 기반 동작
    }
}
