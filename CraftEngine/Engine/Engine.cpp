#include <pch.h>
#include "Engine.h"
#include <Level/Level.h>
#include <Core/Input.h>
#include <Render/Renderer.h>
#include <Physics/CollisionSystem.h>
#include <Math/MathUtility.h>
#include <SoundSystem/Sound.h>

namespace Craft
{
	Engine* Engine::instance = nullptr;

	Engine& Engine::Get()
	{
		assert(instance != nullptr && "엔진 인스턴스가 Null이면 안됨");
		return *instance;
	}

	Engine::Engine()
	{
        assert(instance == nullptr);
        instance = this;

        LoadSettings();
        FMath::SeedRandomDevice();

        input = std::make_unique<Input>();
        renderer = std::make_unique<Renderer>(Vector2(setting.width, setting.height));
        collision = std::make_unique<CollisionSystem>();
        sound = std::make_unique<Sound>();
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
                ProcessCollision();  // Tick에서 계산한 액터 변화를 반영하고, 그리기 전에 충돌 판정
                Draw();

                // Level Handling
                if (subLevel)
                {
                    if (mainLevel)
                    {
                        mainLevel->EndPlay();
                        mainLevel.reset();
                    }

                    mainLevel = subLevel;

                    subLevel.reset();
                }

                if (mainLevel)
                {
                    mainLevel->ProcessRequestedActors();
                    mainLevel->SavePreviousActorStates();
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

    void Engine::PlayOneShot(const std::string& filename)
    {
        if (!sound)
        {
            return;
        }

        // 사운드 에셋 경로 concat
        sound->PlayOneShot("../Content/Sound/" + filename);
    }

    void Engine::PlayBGM(const std::string& filename)
    {
        if (!sound)
        {
            return;
        }

        sound->PlayBackgroundMusic("../Content/Sound/" + filename);
    }

    void Engine::StopBGM()
    {
        if (!sound)
        {
            return;
        }

        sound->StopBackgroundMusic();
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

        mainLevel->Draw();  // 레벨에 속한 액터들의 Draw를 호출해 RenderQueue에 Submit

        renderer->Draw();  // 위에서 제출한 데이터를 기반으로 실제 드로우 시행
	}

    void Engine::ProcessCollision()
    {
        if (!mainLevel)
        {
            return;
        }

        // Engine은 Level 쪽에 friend 선언되어 접근 가능
        // 즉 Level - Engine - Collision 구조로 Engine이 중재자
        collision->ProcessCollision(mainLevel->actors);
    }

	void Engine::SavePreviousInputStates()
	{
        assert(input != nullptr);
        input->SaveKeyStates();
	}

	void Engine::Shutdown()
	{

	}

    void Engine::LoadSettings()
    {
        std::ifstream file("../Config/Setting.txt");
        assert(file.is_open());

        std::string line{};
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            const size_t equalPos = line.find('=');
            assert(equalPos != std::string::npos);

            auto Trim = [](std::string& str) -> bool
                {
                    const char* whitespace = " \t\r\n";

                    const size_t begin = str.find_first_not_of(whitespace);
                    if (begin == std::string::npos)
                    {
                        str.clear();
                        return false;
                    }

                    const size_t end = str.find_last_not_of(whitespace);

                    str = str.substr(begin, end - begin + 1);
                    return true;
                };

            std::string key = line.substr(0, equalPos); Trim(key);
            std::string value = line.substr(equalPos + 1); Trim(value);

            assert(!key.empty() && !value.empty());

            if (key == "framerate")
            {
                setting.framerate = (float)atof(value.c_str());
                assert(setting.framerate > 0.f);
            }
            else if (key == "width")
            {
                setting.width = atoi(value.c_str());
                assert(setting.width > 0);
            }
            else if (key == "height")
            {
                setting.height = atoi(value.c_str());
                assert(setting.height > 0);
            }
        }

        file.close();
	}
}