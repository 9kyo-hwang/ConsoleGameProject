#include <pch.h>
#include "Renderer.h"

namespace Craft
{
    Renderer* Renderer::_instance = nullptr;

    Renderer& Renderer::Get()
    {
        assert(_instance != nullptr);
        return *_instance;
    }

    Renderer::Renderer()
    {
        assert(_instance == nullptr);
        _instance = this;

        _consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

        // 콘솔 화면에 깜빡이는 커서가 안보이도록
        CONSOLE_CURSOR_INFO info{ 1, false };  // 사이즈 제일 작게, visible = false
        SetConsoleCursorInfo(_consoleOutput, &info);
    }

    Renderer::~Renderer()
    {
        _instance = nullptr;

        // 콘솔 커서 다시 보이도록
        CONSOLE_CURSOR_INFO info{ 1, true };
        SetConsoleCursorInfo(_consoleOutput, &info);
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
        system("cls");  // clear screen
    }

    void Renderer::DrawRenderQueue()
    {
        for (const RenderCommand& command : _renderQueue)
        {
            // TODO: 이중 버퍼 구현 시 sortingOrder 비교 후 그릴 지 판단

            // 콘솔 좌표 이동
            COORD cursorPosition{ (SHORT)command.position.x, (SHORT)command.position.y };
            SetConsoleCursorPosition(
                _consoleOutput,
                cursorPosition
            );

            // 색상 설정
            SetConsoleTextAttribute(_consoleOutput, (WORD)command.color);

            // 이미지(문자열) 그림
            std::cout << command.image;

            // 콘솔 색상 원복
            SetConsoleTextAttribute(_consoleOutput, (WORD)Color::White);
        }

        // 큐 비우기
        _renderQueue.clear();
    }

    void Renderer::Present()
    {
        // TODO: 이중 버퍼 구현 시 사용
    }
}