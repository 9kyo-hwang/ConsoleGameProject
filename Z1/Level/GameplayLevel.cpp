#include "pch.h"
#include "GameplayLevel.h"
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Game/Game.h>
#include <Component/SpriteRendererComponent.h>
#include <Actor/Actor.h>

using namespace Craft;

namespace
{
    std::shared_ptr<const Sprite> CreateTestSprite()
    {
        std::vector<SpriteCell> cells
        {
            {' ', 0, true},
            {'^', (WORD)Color::Green, false},
            {' ', 0, true},
            {' ', 0, true},
            {' ', 0, true},

            {'<', (WORD)Color::Yellow, false},
            {'#', (WORD)Color::Red, false},
            {'>', (WORD)Color::Yellow, false},
            {' ', 0, true},
            {' ', 0, true},

            {' ', 0, true},
            {'v', (WORD)Color::Green, false},
            {' ', 0, true},
            {' ', 0, true},
            {' ', 0, true},
        };

        return std::make_shared<const Sprite>(
            Vector2(5, 3),
            std::move(cells)
        );
    }
}

GameplayLevel::GameplayLevel()
{
    _testSprite = CreateTestSprite();
}

void GameplayLevel::BeginPlay()
{
    Level::BeginPlay();

    if (_spriteActor)
    {
        return;
    }

    // Actor의 Render는 컴포넌트에 의해.
    _spriteActor = SpawnActor<Actor>(Vector2(-1, -1));
    _spriteActor->AddComponent<SpriteRendererComponent>(_testSprite, 10);
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
