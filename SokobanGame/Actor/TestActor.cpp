#include <pch.h>
#include "TestActor.h"
#include <Core/Input.h>

using namespace Craft;

void TestActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    // ESC키 종료
    if(Input::Get().GetKeyDown(VK_ESCAPE))
    {
        QuitGame();
    }

    if (Input::Get().GetKeyDown('A'))
    {
        std::cout << "GetKeyDown('A')\n";
    }

    if (Input::Get().GetKey('A'))
    {
        std::cout << "GetKey('A')\n";
    }

    if (Input::Get().GetKeyUp('A'))
    {
        std::cout << "GetKeyUp('A')\n";
    }

    //std::cout << "TestActor::Tick() - deltaTime: "
    //    << deltaTime
    //    << " | FPS: "
    //    << 1.f / deltaTime
    //    << "\n";
}
