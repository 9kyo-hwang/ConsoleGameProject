#include <pch.h>
#include "Actor.h"
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Render/Renderer.h>

namespace Craft
{
	Actor::Actor(const std::string& image, const Vector2& position, Color color)
        : image(image)
        , position(position)
        , color(color)
        , width((int32)image.length())
	{
        // 트랜스폼은 생성자에서 직접 생성
        transform = std::make_shared<TransformComponent>(position);
	}

    Actor::Actor(const Vector2& position)
        : Actor("", position, Color::White)
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
        if (!IsActive())
        {
            return;
        }

        // Renderer에 이 액터의 정보 제출
        Renderer::Get().Submit(image, position, color, sortingOrder);
	}

	void Actor::OnCollision(const std::shared_ptr<Actor>& other)
	{
	}

    void Actor::ChangeImage(const std::string& newImage)
    {
        width = (int)newImage.size();
        image = newImage;
    }

    void Actor::Destroy()
	{
		hasExpired = true;
	}

	void Actor::QuitGame()
	{
		Engine::Get().Quit();
	}

    void Actor::SavePreviousStates()
    {
        previousPosition = position;
    }

	void Actor::SetPosition(Vector2 newPosition)
	{
        if (position == newPosition)
        {
            return;
        }

        position = newPosition;
	}

    void Actor::ProcessAddRequestedComponents()
    {
        if (addRequestedComponents.empty())
        {
            return;
        }

        for (auto& component : addRequestedComponents)
        {
            if (!component)
            {
                continue;
            }

            components.emplace_back(component);

            // Actor는 BeginPlay 했는데 Component가 아직이라면
            if (HasBeganPlay() && !component->HasBeganPlay())
            {
                component->BeginPlay();
            }
        }

        SetComponentOwners();
        addRequestedComponents.clear();
    }

    void Actor::SetComponentOwners()
    {
        std::shared_ptr<Actor> thisActor = shared_from_this();
        if (!thisActor)
        {
            return;
        }

        if (transform)
        {
            transform->SetOwner(thisActor);
        }

        for (const auto& component : components)
        {
            if (component)
            {
                component->SetOwner(thisActor);
            }
        }

        for (const auto& component : addRequestedComponents)
        {
            if (component)
            {
                component->SetOwner(thisActor);
            }
        }
    }
}