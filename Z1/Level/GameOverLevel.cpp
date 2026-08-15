#include "pch.h"
#include "GameOverLevel.h"

#include <Core/Input.h>
#include <Game/Game.h>
#include <Render/Renderer.h>

using namespace Craft;

void GameOverLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (Input::Get().GetKeyDown(VK_RETURN) ||
        Input::Get().GetKeyDown('A'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Title);
    }
}

void GameOverLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit("GAME OVER", Vector2(30, 20));
    Renderer::Get().Submit("Press A", Vector2(30, 22));
}
