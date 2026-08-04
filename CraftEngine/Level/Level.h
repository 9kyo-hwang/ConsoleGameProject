#pragma once

#include <vector>
#include <memory>
#include <Core/Core.h>
#include <Actor/Actor.h>

namespace Craft
{
    template<typename T>
    concept ActorType = std::derived_from<T, Actor>;
	
	class CRAFT_API Level : public std::enable_shared_from_this<Level>
	{
        friend class Engine;    // Engine은 Level의 모든 것을 알 수 있음

	public:
		Level();
		virtual ~Level();

        virtual void OnInitialized();

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void Draw();

        inline bool HasInitialized() const { return hasInitialized; }

		template<ActorType T, typename... Args>
		std::shared_ptr<T> SpawnActor(Args&&... args)
		{
			auto actor = std::make_shared<T>(std::forward<Args>(args)...);
			addRequestedActors.emplace_back(actor);

			actor->SetOwner(weak_from_this());

			return actor;
		}

        template<ActorType T>
        std::shared_ptr<T> SpawnActor(const TSubclassOf<T>& ActorClass)
        {
            auto ActorInstance = ActorClass.New();
            if (!ActorInstance)
            {
                return nullptr;
            }

            addRequestedActors.emplace_back(ActorInstance);
            ActorInstance->SetOwner(weak_from_this());

            return ActorInstance;
        }

		template<ActorType T>
		std::shared_ptr<T> FindActor()
		{
			// TypeCasting
			for (const auto& actor : actors)
			{
				if (auto target = std::dynamic_pointer_cast<T>(actor))
				{
					return target;
				}
			}

			return nullptr;
		}

	protected:
		void ProcessRequestedActors();

	protected:
		bool hasInitialized = false;
		std::vector<std::shared_ptr<Actor>> actors;
		
		// 다음 프레임에 추가될 액터들
		// 현재 프레임에 처리하는 액터 목록에 변화가 생기면 문제가 생길 수 있음
		std::vector<std::shared_ptr<Actor>> addRequestedActors;	
	};
}
