#include <pch.h>
#include "Renderer.h"
#include <Render/ScreenBuffer.h>

namespace Craft
{
    Renderer::Frame::Frame(int32 bufferCount)
    {
        // 2차원 배열(글자/그리기 순서) 생성
        charInfos = std::make_unique<CHAR_INFO[]>(bufferCount);
        sortingOrders = std::make_unique<int32[]>(bufferCount);
    }

    Renderer::Frame::~Frame()
    {
    }

    void Renderer::Frame::Clear(const Vector2& size)
    {
        const int32 width = size.x;
        const int32 height = size.y;

        for (int32 y = 0; y < height; ++y)
        {
            for (int32 x = 0; x < width; ++x)
            {
                // 2D -> 1D
                const int32 index = (y * width) + x;

                CHAR_INFO& info = charInfos[index];
                info.Char.AsciiChar = ' ';  // Unicode X
                info.Attributes = 0;

                sortingOrders[index] = -1;
            }
        }
    }

    /*************************/

    Renderer* Renderer::_instance = nullptr;

    Renderer& Renderer::Get()
    {
        assert(_instance != nullptr);
        return *_instance;
    }

    Renderer::Renderer(const Vector2& screenSize)
        : _screenSize(screenSize)
    {
        assert(_instance == nullptr);
        _instance = this;

        // 스크린 프론트/백 버퍼 생성
        _screenBuffers[0] = std::make_unique<ScreenBuffer>(screenSize);
        _screenBuffers[0]->Clear();

        _screenBuffers[1] = std::make_unique<ScreenBuffer>(screenSize);
        _screenBuffers[1]->Clear();

        // 0번 콘솔 버퍼(프론트)를 지정
        BOOL result = SetConsoleActiveScreenBuffer(_screenBuffers[0]->GetHandle());
        assert(result == TRUE);

        // 프레임 생성
        const int32 bufferCount = _screenSize.x * _screenSize.y;
        _frame = std::make_unique<Frame>(bufferCount);
        _frame->Clear(_screenSize);
    }

    Renderer::~Renderer()
    {
        _instance = nullptr;

        // 표준 콘솔로 복구
        SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
    }

    void Renderer::Submit(const std::string& image, const Vector2& position, Color color, int sortingOrder)
    {
        // 액터가 렌더러에게 그릴 데이터를 전달해줌
        RenderCommand command
        {
            .image = image, 
            .position = position, 
            .color = color, 
            .sortingOrder = sortingOrder
        };

        _renderQueue.emplace_back(command);
    }

    void Renderer::Draw()
    {
        Clear();

        DrawRenderQueue();

        Present();
    }

    void Renderer::Clear()
    {
        // 화면 지우는 콘솔 명령어 실행
        //system("cls");  // clear screen

        // 프레임 초기화
        _frame->Clear(_screenSize);

        // 콘솔 버퍼 초기화
        GetCurrentScreenBuffer()->Clear();
    }

    void Renderer::DrawRenderQueue()
    {
        for (const RenderCommand& command : _renderQueue)
        {
            if (command.image.empty())
            {
                continue;
            }
            
            // OutOfBound - y
            const Vector2& pos = command.position;
            if (pos.y < 0 || pos.y >= _screenSize.y)
            {
                continue;
            }

            const int32 length = (int32)command.image.length();
            const int32 imageStartX = pos.x;
            const int32 imageEndX = imageStartX + length - 1;

            // OutOfBound - x
            if (imageEndX < 0 || imageStartX >= _screenSize.x)
            {
                continue;
            }

            // 글자가 잘리는 경우를 대비해 실제로 보여질 index 계산
            const int32 visibleStartX = std::max<int32>(0, imageStartX);
            const int32 visibleEndX = std::min<int32>(_screenSize.x - 1, imageEndX);

            for (int32 x = visibleStartX; x <= visibleEndX; ++x)
            {
                const int32 sourceIndex = x - imageStartX;          // 문자 인덱스
                const int32 index = (pos.y * _screenSize.x) + x;    // 문자 기록용 2차원 배열 인덱스

                // [pos.y][x] 위치의 글자를 변경할지 확인
                if (_frame->sortingOrders[index] > command.sortingOrder)
                {
                    continue;
                }

                _frame->charInfos[index].Char.AsciiChar = command.image[sourceIndex];
                _frame->charInfos[index].Attributes = (WORD)command.color;
                _frame->sortingOrders[index] = command.sortingOrder;
            }
        }

        // 백버퍼에 그리기
        GetCurrentScreenBuffer()->Draw(_frame->charInfos.get());

        // 큐 비우기
        _renderQueue.clear();

        // 콘솔 초기화
        SetConsoleTextAttribute(
            GetCurrentScreenBuffer()->GetHandle(),
            (WORD)Color::White
        );
    }

    void Renderer::Present()
    {
        SetConsoleActiveScreenBuffer(GetCurrentScreenBuffer()->GetHandle());

        _currentBufferIndex = 1 - _currentBufferIndex;
    }

    ScreenBuffer* const Renderer::GetCurrentScreenBuffer() const
    {
        return _screenBuffers[_currentBufferIndex].get();
    }
}