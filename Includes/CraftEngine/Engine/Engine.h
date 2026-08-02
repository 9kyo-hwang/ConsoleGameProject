#pragma once

#include <memory>
#include <Core/Core.h>

namespace Craft
{
    class Level;
    class Input;

	// 메인 엔진 클래스.
	// 엔진 루프를 제공.
	// 게임 엔진의 핵심 기능 제공.
	class CRAFT_API Engine
	{
		// 엔진 설정 (데이터).
		struct Setting
		{
			// 목표 프레임 수 (초당 프레임).
			float framerate = 120.0f;
		};

	public:
		static Engine& Get();

        template<typename LevelType>
        void AddNewLevel() requires std::is_base_of_v<Level, LevelType>
        {
            subLevel = std::make_shared<LevelType>();

            // main level(levels[0]) 처리 끝나면 sub level(levels[1])을 main으로 승격
        }

	public:
		Engine();
		virtual ~Engine();

		// 엔진 실행 함수.
		void Run();

		// 엔진 종료 함수.
		void Quit();

	protected:
		// 입력 처리 함수 (입력 폴링).
		void ProcessInput();

		// 초기화 함수.
		void OnInitialized();

		// 게임 플레이 이벤트 함수.

		// 게임 플레이 초기화 함수.
		void BeginPlay();

		// 게임 플레이 업데이트 함수.
		void Tick(float deltaTime);

		// 레벨 그리기 함수.
		void Draw();

		// 프레임 간 입력 값 저장을 위한 함수.
		void SavePreviousInputStates();

		// 엔진 종료 시 정리가 필요할 때 사용할 함수.
		void Shutdown();

	protected:
		// 엔진 종료 요청 여부 플래그.
		bool isQuit = false;

		// 엔진 설정 함수.
		Setting setting;

		static Engine* instance;

        std::shared_ptr<Level> mainLevel;
        std::shared_ptr<Level> subLevel;

        std::unique_ptr<Input> input;
	};
}
