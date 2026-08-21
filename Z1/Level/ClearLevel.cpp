#include "pch.h"
#include "ClearLevel.h"
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Game/Game.h>

using namespace Craft;

ClearLevel::ClearLevel()
{
}

void ClearLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/15 Ending Theme.wav");
        _bgmStarted = true;
    }
}

void ClearLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (Input::Get().GetKeyDown(VK_RETURN))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Title);
    }
}

void ClearLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit("CLEAR", Craft::Vector2(30, 20));
    Renderer::Get().Submit("Press Enter", Craft::Vector2(30, 22));
}

void ClearLevel::EndPlay()
{
    Level::EndPlay();

    Engine::Get().StopBGM();
    _bgmStarted = false;
}
