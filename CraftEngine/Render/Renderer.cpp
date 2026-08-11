#include "pch.h"
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
        TextPayload payload
        {
            .text = image,
            .color = color,
        };

        RenderCommand command
        {
            .payload = payload,
            .position = position, 
            .sortingOrder = sortingOrder
        };

        _renderQueue.emplace_back(command);
    }

    void Renderer::Submit(std::shared_ptr<const Sprite> sprite, const Vector2& position, const Vector2& cellScale, int sortingOrder)
    {
        assert(cellScale.x > 0 && cellScale.y > 0);

        SpritePayload payload
        {
            .sprite = std::move(sprite),
            .cellScale = cellScale,
        };

        RenderCommand command
        {
            .payload = payload,
            .position = Vector2(position.x * cellScale.x, position.y * cellScale.y),
            .sortingOrder = sortingOrder
        };

        _renderQueue.emplace_back(std::move(command));
    }

    void Renderer::Submit(std::shared_ptr<const Sprite> sprite, const Vector2& position, int sortingOrder)
    {
        Submit(sprite, position, Vector2::One, sortingOrder);
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
        // command가 들고 있는 Payload를 보고
        // DrawPayload -> DrawCellGrid -> CompositeCell -> 
        for (const RenderCommand& command : _renderQueue)
        {
            // variant 내부의 현재 활성화된 타입을 안전하게 판별하고
            // 그에 맞는 Callable을 실행
            std::visit([&](const auto& payload)
                {
                    DrawPayload(command, payload);
                },
                command.payload
            );
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

    void Renderer::DrawPayload(const RenderCommand& command, const TextPayload& payload)
    {
        if (payload.text.empty())
        {
            return;
        }

        // N x 1 문자열 이미지를 SpriteCell로 치환
        DrawCellGrid(
            command.position,
            Vector2((int)payload.text.size(), 1),
            command.sortingOrder,
            [&](int x, int)
            {
                return SpriteCell
                {
                    .glyph = payload.text[x],
                    .attributes = (WORD)(payload.color),
                    .transparent = false
                };
            }
        );
    }

    void Renderer::DrawPayload(const RenderCommand& command, const SpritePayload& payload)
    {
        if (!payload.sprite)
        {
            return;
        }

        const Sprite& sprite = *payload.sprite;
        const Vector2 logicalSize = sprite.GetSize();

        // 실제 픽셀이 차지할 크기는 cellScale에 비례
        const Vector2 physicalSize(logicalSize.x * payload.cellScale.x, logicalSize.y * payload.cellScale.y);

        DrawCellGrid(
            command.position,
            physicalSize,
            command.sortingOrder,
            [&](int physicalX, int physicalY)
            {
                const int logicalX = physicalX / payload.cellScale.x;
                const int logicalY = physicalY / payload.cellScale.y;
                return sprite.GetCell(logicalX, logicalY);
            }
        );
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