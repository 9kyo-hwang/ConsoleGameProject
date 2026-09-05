#include "pch.h"
#include "DevelopmentLevel.h"
#include <Render/Renderer.h>

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
}

void DevelopmentLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);
}

void DevelopmentLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit(Sprite::Create("[Development Level]"), Vector2::Zero);
}