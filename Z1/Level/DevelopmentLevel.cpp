#include "pch.h"
#include "DevelopmentLevel.h"
#include <Actor/CollisionTestActor.h>
#include <Render/Renderer.h>
#include <Core/Input.h>

using namespace Craft;

DevelopmentLevel::DevelopmentLevel()
{
}

void DevelopmentLevel::OnInitialized()
{
    Level::OnInitialized();
}

void DevelopmentLevel::BeginPlay()
{
    Level::BeginPlay();

    if (_testA && _testB)
    {
        return;
    }

    _testA = SpawnActor<CollisionTestActor>(
        Vector2(10, 10),
        Vector2::One,
        Vector2::Zero
    );

    _testB = SpawnActor<CollisionTestActor>(
        Vector2(15, 10),
        Vector2::One,
        Vector2::Zero
    );
}

void DevelopmentLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (Input::Get().GetKeyDown('M'))
    {
        _testA->SetPosition(Vector2(20, 10));
    }
}

void DevelopmentLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit("[Development Level]", Vector2::Zero);
    Renderer::Get().Submit("CollisionCount of A: " + std::to_string(_testA->GetCollisionCount()), Vector2(35, 1));
    Renderer::Get().Submit("CollisionCount of B: " + std::to_string(_testB->GetCollisionCount()), Vector2(35, 2));
}
