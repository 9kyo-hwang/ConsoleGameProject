#include "pch.h"
#include "GameOverLevel.h"

#include <Core/Input.h>
#include <Game/Game.h>
#include <Render/Renderer.h>

using namespace Craft;

void GameOverLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/11 Game Over.wav");
        _bgmStarted = true;
    }
}

void GameOverLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (Input::Get().GetKeyDown(VK_RETURN))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Title);
    }
}

void GameOverLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit(Sprite::Create("GAME OVER"), Vector2(30, 20));
    Renderer::Get().Submit(Sprite::Create("Press Enter"), Vector2(30, 22));
}

void GameOverLevel::EndPlay()
{
    Level::EndPlay();

    Engine::Get().StopBGM();
    _bgmStarted = false;
}
