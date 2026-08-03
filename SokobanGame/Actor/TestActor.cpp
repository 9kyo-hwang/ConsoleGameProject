#include <pch.h>
#include "TestActor.h"
#include <Core/Input.h>

using namespace Craft;

TestActor::TestActor()
    : Super("P", Vector2(5, 5), Color::Green)
{
    sortingOrder = 5;
}

void TestActor::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // ESC키 종료
    if(Input::Get().GetKeyDown(VK_ESCAPE))
    {
        QuitGame();
    }

    if (Input::Get().GetKey(VK_LEFT) && position.x > 0)
    {
        position.x -= 1;
    }

    // TEMP: 화면 크기 임시값으로 제한
    if (Input::Get().GetKey(VK_RIGHT) && position.x < 39)
    {
        position.x += 1;
    }

    if (Input::Get().GetKey(VK_UP) && position.y > 0)
    {
        position.y -= 1;
    }

    if (Input::Get().GetKey(VK_DOWN) && position.y < 24)
    {
        position.y += 1;
    }
}
