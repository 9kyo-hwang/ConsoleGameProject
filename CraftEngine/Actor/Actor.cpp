#include <pch.h>
#include "Actor.h"
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Render/Renderer.h>

namespace Craft
{
    Actor::Actor(const Vector2& position)
    {
        // 트랜스폼은 생성자에서 직접 생성
        transform = std::make_shared<TransformComponent>(position);
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
        if (!IsActive())
        {
            return;
        }

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

        // Draw를 정의한(SpriteRenderer 등) 컴포넌트가 수행
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

    void Actor::EndPlay()
    {
        if (!IsActive())
        {
            return;
        }

        for (const auto& component : components)
        {
            component->EndPlay();
        }
    }

    void Actor::Destroy()
	{
		hasExpired = true;

        // 자식 Actor의 파괴 결정은 여전히 Actor의 책임
        for (const auto& childTransform : transform->GetChildren())
        {
            if (auto childActor = childTransform->GetOwner())
            {
                childActor->Destroy();
            }
        }
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

    void Actor::AttachTo(const std::shared_ptr<Actor>& newParent, bool keepWorldPosition)
    {
        if (!newParent || !transform || !newParent->transform)
        {
            return;
        }

        transform->AttachTo(newParent->transform, keepWorldPosition);
    }

    void Actor::DetachFromParent()
    {
        if (!transform)
        {
            return;
        }

        transform->DetachFromParent();
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

        if (transform)
        {
            transform->SetLocalPosition(newPosition);
        }
	}

    Vector2 Actor::GetPreviousPosition() const
    {
        if (transform)
        {
            return transform->GetPreviousWorldPosition();
        }

        return Vector2::Zero;
    }

    void Actor::ProcessAddRequestedComponents()
    {
        if (addRequestedComponents.empty())
        {
            return;
        }

        SetComponentOwners();
        for (const auto& component : addRequestedComponents)
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