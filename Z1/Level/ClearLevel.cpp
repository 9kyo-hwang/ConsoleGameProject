#include "pch.h"
#include "ClearLevel.h"
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Game/Game.h>

using namespace Craft;

ClearLevel::ClearLevel()
{
}

void ClearLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (Input::Get().GetKeyDown('1'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Title);
    }

    if (Input::Get().GetKeyDown('2'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Overworld);
    }

    if (Input::Get().GetKeyDown('3'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Clear);
    }
}

void ClearLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit("CLEAR", Craft::Vector2::Zero);
}
