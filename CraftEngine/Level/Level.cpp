#include <pch.h>
#include "Level.h"
#include <Actor/Actor.h>

namespace Craft
{
	Level::Level()
	{

	}

	Level::~Level()
	{

	}

    void Level::OnInitialized()
    {
        hasInitialized = true;
    }

	void Level::BeginPlay()
	{
		for (const auto& actor : actors)
		{
			if (!actor->HasBeganPlay())
			{
				actor->BeginPlay();
			}
		}
	}

	void Level::Tick(float deltaTime)
	{
		for (const auto& actor : actors)
		{
			if (actor->IsActive())
			{
				actor->Tick(deltaTime);
			}
		}
	}

	void Level::Draw()
	{
		for (const auto& actor : actors)
		{
			if (actor->IsActive())
			{
				actor->Draw();
			}
		}
	}

	// 이전 프레임에서 추가/삭제 요청한 액터 목록을 현재 프레임에서 처리
	void Level::ProcessRequestedActors()
	{
		for (auto it = actors.begin(); it != actors.end();)
		{
			if (*it != nullptr && (*it)->HasExpired())
			{
				it = actors.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto& actor : addRequestedActors)
		{
			actors.push_back(std::move(actor));
		}

		addRequestedActors.clear();
	}
}