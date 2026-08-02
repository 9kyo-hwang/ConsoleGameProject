#include <pch.h>
#include "Engine.h"
#include <Level/Level.h>
#include <Core/Input.h>

namespace Craft
{
	Engine* Engine::instance = nullptr;

	Engine& Engine::Get()
	{
		// TODO: 여기에 return 문을 삽입합니다.
		assert(instance != nullptr, "엔진 인스턴스가 Null이면 안됨");
		return *instance;
	}

	Engine::Engine()
	{
        assert(instance == nullptr);
        instance = this;

        input = std::make_unique<Input>();
	}

	Engine::~Engine()
	{
        instance = nullptr;
	}

	void Engine::Run()
	{
		const float oneFrameTime = 1.f / setting.framerate;

		__int64 frequency{};
		::QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);

		__int64 previousCount{};
		::QueryPerformanceCounter((LARGE_INTEGER*)&previousCount);

		while (!isQuit)
		{
			__int64 currentCount{};
			::QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);

			float deltaTime
				= static_cast<float>(currentCount - previousCount) / frequency;

			if (deltaTime >= oneFrameTime)
			{
				ProcessInput();

                // Gameplay Logic
				OnInitialized();
				BeginPlay();
				Tick(deltaTime);
				Draw();

                // Level Handling
                if (subLevel)
                {
                    if (mainLevel)
                    {
                        mainLevel.reset();
                    }

                    mainLevel = subLevel;

                    subLevel.reset();
                }

                if (mainLevel)
                {
                    mainLevel->ProcessRequestedActors();
                }

				SavePreviousInputStates();

				previousCount = currentCount;
			}
		}

		Shutdown();
	}

	void Engine::Quit()
	{
		isQuit = true;
	}

	void Engine::ProcessInput()
	{
        assert(input != nullptr);
        input->ProcessInput();
	}

	void Engine::OnInitialized()
	{
        // Level Initialize
        if (!mainLevel || mainLevel->HasInitialized())
        {
            return;
        }

        mainLevel->OnInitialized();
	}

	void Engine::BeginPlay()
	{
        if (!mainLevel)
        {
            return;
        }

        mainLevel->BeginPlay();
	}

	void Engine::Tick(float deltaTime)
	{
        if (!mainLevel)
        {
            return;
        }

        mainLevel->Tick(deltaTime);
	}

	void Engine::Draw()
	{
        if (!mainLevel)
        {
            return;
        }

        mainLevel->Draw();
	}

	void Engine::SavePreviousInputStates()
	{
        assert(input != nullptr);
        input->SaveKeyStates();
	}

	void Engine::Shutdown()
	{

	}
}