#pragma once

#include <vector>
#include <memory>
#include <Core/Core.h>

namespace Craft
{
	class Actor;
	
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

		template<typename ActorType, typename... Args>
		std::shared_ptr<ActorType> SpawnActor(Args&&... args) requires std::is_base_of_v<Actor, ActorType>
		{
			auto actor = std::make_shared<ActorType>(std::forward<Args>(args)...);
			addRequestedActors.emplace_back(actor);

			// TODO: SetOwner. UE에서는 부모를 타고 올라가서 서버/클라/'내' 클라 등등 구분
			actor->SetOwner(weak_from_this());

			return actor;
		}

		template<typename ActorType>
		std::shared_ptr<ActorType> FindActor() requires std::is_base_of_v<Actor, ActorType>
		{
			// TypeCasting
			for (const auto& actor : actors)
			{
				if (auto target = std::dynamic_pointer_cast<ActorType>(actor))
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
