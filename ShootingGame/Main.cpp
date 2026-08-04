#include <pch.h>
#include <Engine/Engine.h>
#include <Level/GameLevel.h>

int main()
{
    Craft::Engine engine;
    engine.AddNewLevel<GameLevel>();
    engine.Run();

    return 0;
}