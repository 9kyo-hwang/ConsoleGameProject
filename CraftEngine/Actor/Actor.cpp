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

    void Actor::Destroy()
	{
		hasExpired = true;

        for (const auto& child : children)
        {
            if (auto childActor = child.lock())
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
        if (!newParent || newParent.get() == this)
        {
            return;
        }

        // 기존 부모 정보 제거
        DetachFromParent();

        // 새 부모 설정
        parent = newParent;
        newParent->children.emplace_back(weak_from_this());

        // transform 부모 갱신
        if (transform && newParent->GetTransform())
        {
            // 기존 월드 좌표 유지를 위해 복사
            Vector2 prev = transform->GetWorldPosition();
            transform->SetParent(newParent->GetTransform());
            if (keepWorldPosition)
            {
                transform->SetWorldPosition(prev);
            }
        }
    }

    void Actor::DetachFromParent()
    {
        if (auto prevParent = GetParent())
        {
            auto& sibling = prevParent->children;
            for (auto it = sibling.begin(); it != sibling.end(); ++it)
            {
                // 가정: 부모의 children에는 내 정보가 1개만 들어있음
                if ((*it).lock().get() == this)
                {
                    it = sibling.erase(it);
                    break;
                }
            }
        }

        parent.reset(); // 기존 부모 참조 reelase
        if (transform)
        {
            Vector2 pos = transform->GetWorldPosition();
            transform->SetParent(std::weak_ptr<TransformComponent>());  // Empty
            transform->SetWorldPosition(pos);   // 이전 월드 포지션 유지
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