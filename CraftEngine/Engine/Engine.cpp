#include "Engine.h"

#include <iostream>
#include <Windows.h>

namespace Craft
{
	Engine::Engine()
	{
	}
	Engine::~Engine()
	{
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

				OnInitialized();
				BeginPlay();
				Tick(deltaTime);
				Draw();

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
	}

	void Engine::OnInitialized()
	{
	}

	void Engine::BeginPlay()
	{
	}

	void Engine::Tick(float deltaTime)
	{
		std::cout << "Engine::Tick() - deltaTime: "
			<< deltaTime
			<< " | FPS: "
			<< 1.f / deltaTime
			<< "\n";
	}

	void Engine::Draw()
	{
	}

	void Engine::SavePreviousInputStates()
	{
	}

	void Engine::Shutdown()
	{
	}
}