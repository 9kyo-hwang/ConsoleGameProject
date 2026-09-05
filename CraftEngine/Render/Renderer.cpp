#include "pch.h"
#include "Renderer.h"
#include <Math/Color.h>
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

    void Renderer::Submit(std::shared_ptr<const Sprite> sprite, Vector2 position, int sortingOrder)
    {
        RenderCommand command
        {
            .sprite = std::move(sprite),
            .position = position,   // 위치값엔 scale 적용 X
            .sortingOrder = sortingOrder
        };

        _renderQueue.emplace_back(std::move(command));
    }

    void Renderer::SubmitWorld(std::shared_ptr<const Sprite> sprite, Vector2 worldPosition, int sortingOrder)
    {
        Submit(std::move(sprite), WorldToScreen(worldPosition), sortingOrder);
    }

    void Renderer::Draw()
    {
        Clear();    // frame(CHAR_INFO, sorting order) 초기화
        DrawRenderQueue();  // Sprite 합성 후 ScreenBuffer::Draw() -> WriteConsoleOutputA
        Present();

        _viewWorldOrigin = Vector2::Zero;
        _viewScreenOrigin = Vector2::Zero;
    }

    void Renderer::Clear()
    {
        // 화면 지우는 콘솔 명령어 실행
        //system("cls");  // clear screen

        // 프레임 초기화: CHAR_INFO 및 sorting order 초기화
        _frame->Clear(_screenSize);

        // 콘솔 버퍼 초기화: Draw에서 현재 스크린 버퍼의 Draw(WriteConsoleOutput)가 
        // writeRegion 전체를 덮어씌워서 Clear는 필요 없음
        //GetCurrentScreenBuffer()->Clear();
    }

    void Renderer::DrawRenderQueue()
    {
        // command가 들고 있는 Payload를 보고 DrawSprite -> CompositeCell
        for (const RenderCommand& command : _renderQueue)
        {
            if (!command.sprite)
            {
                continue;
            }

            DrawSprite(*command.sprite, command.position, command.sortingOrder);
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

    void Renderer::DrawSprite(const Sprite& sprite, Vector2 position, int sortingOrder)
    {
        const Vector2 size = sprite.GetSize();

        const int sourceStartX = std::max(0, -position.x);
        const int sourceStartY = std::max(0, -position.y);
        const int sourceEndX = std::min(size.x, _screenSize.x - position.x);
        const int sourceEndY = std::min(size.y, _screenSize.y - position.y);

        if (sourceStartX >= sourceEndX || sourceStartY >= sourceEndY)
        {
            return;
        }

        for (int sourceY = sourceStartY; sourceY < sourceEndY; ++sourceY)
        {
            for (int sourceX = sourceStartX; sourceX < sourceEndX; ++sourceX)
            {
                const int destinationX = position.x + sourceX;
                const int destinationY = position.y + sourceY;

                CompositeCell(destinationX, destinationY, sprite.GetCell(sourceX, sourceY), sortingOrder);
            }
        }
    }

    void Renderer::CompositeCell(int x, int y, const SpriteCell& cell, int sortingOrder)
    {
        if (cell.transparent)
        {
            return;
        }

        const int32 index = y * _screenSize.x + x;

        // [pos.y][x] 위치의 글자를 변경할지 확인
        if (_frame->sortingOrders[index] > sortingOrder)
        {
            return;
        }

        _frame->charInfos[index].Char.AsciiChar = cell.glyph;
        _frame->charInfos[index].Attributes = cell.attributes;
        _frame->sortingOrders[index] = sortingOrder;
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