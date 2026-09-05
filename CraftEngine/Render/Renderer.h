#pragma once

#include <Core/Core.h>
#include <Math/Vector2.h>
#include <Render/Sprite.h>

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
            std::shared_ptr<const Sprite> sprite;
            Vector2 position;
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
        void Submit(std::shared_ptr<const Sprite> sprite, Vector2 position, int sortingOrder = 0 );
        void SubmitWorld(std::shared_ptr<const Sprite> sprite, Vector2 worldPosition, int sortingOrder = 0);

        // 엔진이 호출할 이벤트 함수
        void Draw();

    public:
        inline void SetView(Vector2 worldOrigin, Vector2 screenOrigin = Vector2::Zero)
        {
            _viewWorldOrigin = worldOrigin;
            _viewScreenOrigin = screenOrigin;
        }

    private:
        void Clear();           // 프레임 시작 시 화면을 지우는 함수
        void DrawRenderQueue(); // 전달받은 렌더 커맨드를 활용해 그리는 함수
        void DrawSprite(const Sprite& sprite, Vector2 position, int sortingOrder);
        void CompositeCell(int x, int y, const SpriteCell& cell, int sortingOrder); // 셀 기록
        void Present();         // 이중 버퍼 구현 시 버퍼 스왑하는 함수

        ScreenBuffer* const GetCurrentScreenBuffer() const;  // 백버퍼 Getter

        inline Vector2 WorldToScreen(Vector2 position) const { return position - _viewWorldOrigin + _viewScreenOrigin; }

    private:
        static Renderer* _instance;
        Vector2 _viewWorldOrigin = Vector2::Zero;   // 카메라가 보고있는 영역의 월드 최상단
        Vector2 _viewScreenOrigin = Vector2::Zero;  // 실제 콘솔 화면의 어느 위치부터 표시할지

        // 현재 프레임에 화면에 그릴 데이터
        std::vector<RenderCommand> _renderQueue{};
        Vector2 _screenSize{};

        std::unique_ptr<Frame> _frame;  // 글자/그리기 순서 2차원 배열을 관리하는 프레임 객체
        std::unique_ptr<ScreenBuffer> _screenBuffers[2]{};  // 콘솔 버퍼(Front/Back)
        int _currentBufferIndex = 0;  // 백버퍼 인덱스. Swap
    };
}