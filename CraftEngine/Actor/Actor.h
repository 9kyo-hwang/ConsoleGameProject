#pragma once

#include <memory>
#include <Core/Core.h>
#include <Core/CObject.h>
#include <Math/Vector2.h>
#include <Math/Color.h>
#include <Component/TransformComponent.h>

#include <string>
#include <type_traits>  // std::is_base_of
#include <utility>      // std::forward
#include <vector>       // component container

namespace Craft
{
    template<typename T>
    concept ActorComponentType = std::derived_from<T, ActorComponent>;

	class Level;

    // Component의 owner가 될 수 있도록(shared_from_this())
	class CRAFT_API Actor : public CObject, public std::enable_shared_from_this<Actor>
	{
        friend class Level; // Level은 Actor의 모든 것을 앎

        TYPE_DECLARATIONS(Actor, CObject)

	public:
        Actor(const Vector2& position = Vector2::Zero);

		virtual ~Actor();

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void Draw();
        virtual void OnCollision(const std::shared_ptr<Actor>& other);

		void Destroy();  // 액터 제거
		void QuitGame();  // 게임(엔진) 종료

        void SavePreviousStates();  // 프레임 종료 시 상태(위치) 캡처

        void AttachTo(const std::shared_ptr<Actor>& newParent, bool keepWorldPosition = true);
        void DetachFromParent();

        inline std::shared_ptr<Level> GetOwner() const { return owner.lock(); }
        void SetOwner(std::weak_ptr<Level> newOwner);

        Vector2 GetPosition() const;
        Vector2 GetWorldPosition() const;
        void SetPosition(Vector2 newPosition);

        template<ActorComponentType T, typename ...Args>
        std::shared_ptr<T> AddComponent(Args&&... args)
        {
            // TransformComponent는 제외(별도 생성 처리, Actor가 무조건 가질 예정)
            static_assert(
                !std::is_same_v<TransformComponent, T>,
                "TransformComponent is created by an actor."
            );

            auto component = std::make_shared<T>(std::forward<Args>(args)...);
            addRequestedComponents.emplace_back(component); // 추가 요청
            return component;
        }

        template<ActorComponentType T>
        std::shared_ptr<T> GetComponent() const
        {
            for (const auto& component : components)
            {
                if (auto target = Cast<T>(component))
                {
                    return target;
                }
            }

            for (const auto& component : addRequestedComponents)
            {
                if (auto target = Cast<T>(component))
                {
                    return target;
                }
            }

            return nullptr;
        }

	public:
		inline bool HasBeganPlay() const { return hasBeganPlay; }
		inline bool IsActive() const { return isActive && !HasExpired(); }
		inline bool HasExpired() const { return hasExpired; }

        inline std::shared_ptr<TransformComponent> GetTransform() const { return transform; }

        Vector2 GetPreviousPosition() const;

    protected:
        void ProcessAddRequestedComponents();  // 추가 요청한 컴포넌트 처리
        void SetComponentOwners();          // 컴포넌트들의 오너 설정

	protected:
		bool hasBeganPlay = false;
		bool isActive = true;
		bool hasExpired = false;

		std::weak_ptr<Level> owner;
        std::shared_ptr<TransformComponent> transform;
        std::vector<std::shared_ptr<ActorComponent>> components;
        std::vector<std::shared_ptr<ActorComponent>> addRequestedComponents; // Tick에 Add 요청한 목록들
	};
}
