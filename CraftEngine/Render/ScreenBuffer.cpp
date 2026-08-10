#include <pch.h>
#include "ScreenBuffer.h"

namespace Craft
{
    ScreenBuffer::ScreenBuffer(Vector2 screenSize)
    {
        _handle = CreateConsoleScreenBuffer(
            GENERIC_READ | GENERIC_WRITE, // RW 모두 가능
            FILE_SHARE_READ | FILE_SHARE_WRITE, // RW 모두 가능
            nullptr, // Security Attributes 미사용(기본 정책)
            CONSOLE_TEXTMODE_BUFFER, // 유일한 플래그
            nullptr // 반드시 Null
        );

        assert(_handle != INVALID_HANDLE_VALUE);

        // 창 크기 설정
        SMALL_RECT windowInfo
        {
            .Left = 0,
            .Top = 0,
            .Right = (SHORT)screenSize.x - 1,   // 인덱스 기반이라 1 작아야 함
            .Bottom = (SHORT)screenSize.y - 1
        };

        BOOL result = SetConsoleWindowInfo(
            _handle,   // GENERIC_READ 권한 필요
            true,       // 좌상단을 (0, 0)으로 사용(기본값)
            &windowInfo
        );

        assert(result == TRUE);

        // 스크린 버퍼 크기 설정
        _size = screenSize;
        result = SetConsoleScreenBufferSize(_handle, _size);
        assert(result == TRUE);

        // 콘솔 커서 끄기
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(_handle, &cursorInfo);

        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(_handle, &cursorInfo);
    }

    ScreenBuffer::~ScreenBuffer()
    {
        if (_handle)
        {
            // OS에게 반환 요청
            CloseHandle(_handle);
        }
    }

    void ScreenBuffer::Clear()
    {
        // 공백 문자를 화면 크기 전체에 한 번에 설정

        COORD position{0, 0};
        DWORD numChars;
        BOOL result = FillConsoleOutputCharacterA(
            _handle,    // GENERIC_WRITE 권한 필요
            ' ',        // 콘솔을 채울 문자. 공백
            _size.X * _size.Y,  // 2차원 배열 모두 비우기 위해 개수 지정
            position,   // 문자를 채울 시작 위치
            &numChars   // 콘솔에 채운 글자 수
        );

        assert(result == TRUE);
    }

    void ScreenBuffer::Draw(const CHAR_INFO* const info)
    {
        // 전달된 글자 배열을 화면에 한 번에 설정

        COORD position{ 0, 0 };
        SMALL_RECT writeRegion
        {
            .Left = 0,
            .Top = 0,
            .Right = _size.X - 1,  // -1이 포함되는 게 맞나?  
            .Bottom = _size.Y - 1
        };

        BOOL result = WriteConsoleOutputA(
            _handle,    // GENERIC_WRITE 권한 필요
            info,       // 1차원 배열이지만, 2차원 배열처럼 사용(시작 주소를 넘김)
            _size,      // 한 번에 세팅할 버퍼 크기
            position,   // 시작 지점
            &writeRegion    // 기록된 글자 개수
        );

        assert(result == TRUE);
    }
}