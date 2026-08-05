#pragma once

#include <memory>
#include <Core/Core.h>
#include <Core/CObject.h>
#include <Math/Vector2.h>
#include <Math/Color.h>
#include <string>

namespace Craft
{
	class Level;

	class CRAFT_API Actor : public CObject
	{
        TYPE_DECLARATIONS(Actor, CObject)

	public:
		Actor(
            const std::string& image = "", 
            const Vector2& position = Vector2::Zero,
            Color color = Color::White
        );

		virtual ~Actor();

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void Draw();
        virtual void OnCollision(const std::shared_ptr<Actor>& other);

        void ChangeImage(const std::string& newImage);  // 폭발 이펙트 등을 발생시키기 위해
		void Destroy();  // 액터 제거
		void QuitGame();  // 게임(엔진) 종료

        void SavePreviousStates();  // 프레임 종료 시 상태(위치) 캡처

        inline std::shared_ptr<Level> GetOwner() const { return owner.lock(); }
		void SetOwner(std::weak_ptr<Level> newOwner) { owner = newOwner; }

        inline Vector2 GetPosition() const { return position; }
        void SetPosition(Vector2 newPosition);

	public:
		inline bool HasBeganPlay() const { return hasBeganPlay; }
		inline bool IsActive() const { return isActive && !HasExpired(); }
		inline bool HasExpired() const { return hasExpired; }

        inline Vector2 GetPreviousPosition() const { return previousPosition; }
        inline int GetWidth() const { return width; }

	protected:
		bool hasBeganPlay = false;
		bool isActive = true;
		bool hasExpired = false;

		std::weak_ptr<Level> owner;

        // Render에 필요한 데이터
        std::string image{};
        Color color = Color::White;
        int width = 0;  // 구현의 단순함을 위해 N x 1 크기 그림(문자열)만 갖도록 제한
        int sortingOrder = 0;
        Vector2 position{};
        Vector2 previousPosition{}; // 충돌 처리를 위해 이전 프레임 위치값 저장
	};
}
