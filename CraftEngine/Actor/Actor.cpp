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

        for (const auto& component : components)
        {
            if (!component->HasBeganPlay())
            {
                component->BeginPlay();
            }
        }
	}

	void Actor::Tick(float deltaTime)
	{
        for (const auto& component : components)
        {
            component->Tick(deltaTime);
        }
	}

	void Actor::Draw()
	{
        if (!IsActive())
        {
            return;
        }

        // Renderer에 이 액터의 정보 제출
        Renderer::Get().Submit(image, position, color, sortingOrder);

        for (const auto& component : components)
        {
            component->Draw();
        }
	}

	void Actor::OnCollision(const std::shared_ptr<Actor>& other)
	{
        if (!IsActive())
        {
            return;
        }

        for (const auto& component : components)
        {
            component->OnCollision(other);
        }
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
        //previousPosition = position;
        if (transform)
        {
            // 위치 정보 관련 갱신은 transform의 책임
            transform->SavePreviousWorldPosition();
        }
    }

    void Actor::SetOwner(std::weak_ptr<Level> newOwner)
    {
        owner = newOwner;

        // 액터의 오너 레벨이 바뀌는 건데, 컴포넌트 오너는 왜 바꾸지?
        // level이 바뀌면서 영향을 주나?
        SetComponentOwners();
    }

    Vector2 Actor::GetPosition() const
    {
        if (transform)
        {
            return transform->GetLocalPosition();
        }

        return Vector2::Zero;
    }

    Vector2 Actor::GetWorldPosition() const
    {
        if (transform)
        {
            return transform->GetWorldPosition();
        }

        return Vector2::Zero;
    }

	void Actor::SetPosition(Vector2 newPosition)
	{
        if (GetPosition() == newPosition)
        {
            return;
        }

        //position = newPosition;
        if (transform)
        {
            transform->SetLocalPosition(newPosition);
        }
	}

    Vector2 Actor::GetPreviousPosition() const
    {
        if (transform)
        {
            transform->GetPreviousWorldPosition();
        }

        return Vector2::Zero;
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