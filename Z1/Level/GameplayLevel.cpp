#include "pch.h"
#include "GameplayLevel.h"
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Game/Game.h>

using namespace Craft;

GameplayLevel::GameplayLevel()
{
}

void GameplayLevel::Tick(float deltaTime)
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
        game.ChangeLevel(State::Gameplay);
    }

    if (Input::Get().GetKeyDown('3'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Clear);
    }
}

void GameplayLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit("GAMEPLAY", Craft::Vector2::Zero);
}
