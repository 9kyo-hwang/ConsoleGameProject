#pragma once

#include <Core/Core.h>
#include <Math/Vector2.h>
#include <Math/Color.h>

#include <string>
#include <vector>

namespace Craft
{
    class CRAFT_API Renderer
    {
    private:
        // 화면에 그릴 데이터를 명령으로 모아둘 구조체
        struct RenderCommand
        {
            std::string image{};
            Vector2 position;
            Color color = Color::White;

            // 그리기 정렬 순서
            int sortingOrder = -1;
        };

    public:
        static Renderer& Get();

    public:
        Renderer();
        ~Renderer();

        // 레벨에 속하는 액터들의 렌더 데이터 전달
        void Submit(const std::string& image, const Vector2& position, Color color = Color::White, int sortingOrder = 0);
        
        // 엔진이 호출할 이벤트 함수
        void Draw();

    private:
        void Clear();           // 프레임 시작 시 화면을 지우는 함수
        void DrawRenderQueue(); // 전달받은 렌더 커맨드를 활용해 그리는 함수
        void Present();         // 이중 버퍼 구현 시 버퍼 스왑하는 함수

    private:
        static Renderer* _instance;
        HANDLE _consoleOutput = nullptr;

        // 현재 프레임에 화면에 그릴 데이터
        std::vector<RenderCommand> _renderQueue{};
    };
}