#pragma once

#include <memory>
#include <Core/Core.h>

namespace Craft
{
    class Level;
    class Input;
    class Renderer;
    class CollisionSystem;

	// 메인 엔진 클래스.
	// 엔진 루프를 제공.
	// 게임 엔진의 핵심 기능 제공.
	class CRAFT_API Engine
	{
		// 엔진 설정 (데이터).
		struct Setting
		{
			// 목표 프레임 수 (초당 프레임).
			float framerate = 0.f;

            int width = 0;
            int height = 0;
		};

	public:
		static Engine& Get();

        template<typename LevelType>
        void AddNewLevel() requires std::derived_from<LevelType, Level>
        {
            subLevel = std::make_shared<LevelType>();

            // main level(levels[0]) 처리 끝나면 sub level(levels[1])을 main으로 승격
        }

	public:
		Engine();
		virtual ~Engine();

		void Run(); // 엔진 실행 함수.
		void Quit(); // 엔진 종료 함수.

        inline int GetWidth() const { return setting.width; }
        inline int GetHeight() const { return setting.height; }

	protected:
		
		void ProcessInput();    // 입력 처리 함수 (입력 폴링).
		void OnInitialized();   // 초기화 함수.

		// 게임 플레이 이벤트 함수.
		void BeginPlay();   // 게임 플레이 초기화 함수.
		void Tick(float deltaTime); // 게임 플레이 업데이트 함수.
		void Draw();    // 레벨 그리기 함수.
        void ProcessCollision();  // 충돌 처리 함수
		void SavePreviousInputStates(); // 프레임 간 입력 값 저장을 위한 함수.
		void Shutdown();    // 엔진 종료 시 정리가 필요할 때 사용할 함수.
        void LoadSettings();  // 엔진 설정 로드 함수

	protected:
		// 엔진 종료 요청 여부 플래그.
		bool isQuit = false;

		// 엔진 설정 함수.
		Setting setting;

		static Engine* instance;

        std::shared_ptr<Level> mainLevel;
        std::shared_ptr<Level> subLevel;

        std::unique_ptr<Input> input;
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<CollisionSystem> collision;
	};
}
