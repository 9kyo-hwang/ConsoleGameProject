#pragma once

#include <Core/Core.h>
#include <Math/Vector2.h>
#include <Math/Color.h>

#include <string>
#include <vector>
#include <memory>

namespace Craft
{
    class ScreenBuffer; // 렌더러가 사용
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

        // 화면 프레임(글자 2차원 배열)
        struct Frame
        {
            // 화면 가로 x 세로 크기를 배열 크기로 설정
            Frame(int bufferCount);
            ~Frame();

            // 프레임 초기화
            void Clear(const Vector2& size);
            
            std::unique_ptr<CHAR_INFO[]> charInfos;  // 문자 2차원 배열
            std::unique_ptr<int[]> sortingOrders;  // 그리기 정렬 값 2차원 배열. 두 배열 크기 동일 보장
        };

    public:
        static Renderer& Get();

    public:
        Renderer(const Vector2& screenSize);
        ~Renderer();

        // 레벨에 속하는 액터들의 렌더 데이터 전달
        void Submit(const std::string& image, const Vector2& position, Color color = Color::White, int sortingOrder = 0);
        
        // 엔진이 호출할 이벤트 함수
        void Draw();

    private:
        void Clear();           // 프레임 시작 시 화면을 지우는 함수
        void DrawRenderQueue(); // 전달받은 렌더 커맨드를 활용해 그리는 함수
        void Present();         // 이중 버퍼 구현 시 버퍼 스왑하는 함수

        ScreenBuffer* const GetCurrentScreenBuffer() const;  // 백버퍼 Getter

    private:
        static Renderer* _instance;

        // 현재 프레임에 화면에 그릴 데이터
        std::vector<RenderCommand> _renderQueue{};
        Vector2 _screenSize{};

        std::unique_ptr<Frame> _frame;  // 글자/그리기 순서 2차원 배열을 관리하는 프레임 객체
        std::unique_ptr<ScreenBuffer> _screenBuffers[2]{};  // 콘솔 버퍼(Front/Back)
        int _currentBufferIndex = 0;  // 백버퍼 인덱스. Swap
    };
}