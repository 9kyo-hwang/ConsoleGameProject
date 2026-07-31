#pragma once

#include <memory>

namespace Craft
{
	class Level;

	class Actor
	{
	public:
		Actor();
		virtual ~Actor();

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void Draw();

		// 액터 제거
		void Destroy();

		// 게임(엔진) 종료
		void QuitGame();

		inline std::shared_ptr<Level> GetOwner() const { return owner.lock(); }
		inline void SetOwner(std::weak_ptr<Level> newOwner) { owner = newOwner; }

	public:
		inline bool HasBeganPlay() const { return hasBeganPlay; }
		inline bool IsActive() const { return isActive && !HasExpired(); }
		inline bool HasExpired() const { return hasExpired; }

	protected:
		// BeginPlay 이벤트 처리 여부
		bool hasBeganPlay = false;

		// 액터 활성화 여부
		bool isActive = true;

		// 삭제 요청 여부
		bool hasExpired = false;

		std::weak_ptr<Level> owner;
	};
}
