#include <pch.h>
#include "Actor.h"
#include <Engine/Engine.h>
#include <Level/Level.h>

namespace Craft
{
	Actor::Actor()
	{
	}

	Actor::~Actor()
	{
	}

	void Actor::BeginPlay()
	{
		hasBeganPlay = true;
	}

	void Actor::Tick(float deltaTime)
	{
	}

	void Actor::Draw()
	{
	}

	void Actor::Destroy()
	{
		hasExpired = true;
	}

	void Actor::QuitGame()
	{
		Engine::Get().Quit();
	}
}