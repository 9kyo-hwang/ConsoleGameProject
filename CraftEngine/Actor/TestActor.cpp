#include <pch.h>
#include "TestActor.h"

void TestActor::Tick(float deltaTime)
{
    Craft::Actor::Tick(deltaTime);

    // ESC키 종료
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        QuitGame();
    }

    std::cout << "TestActor::Tick() - deltaTime: "
        << deltaTime
        << " | FPS: "
        << 1.f / deltaTime
        << "\n";
}
