#pragma once

#include <Math/Vector2.h>
#include <Windows.h>

namespace Craft
{
    class ScreenBuffer
    {
    public:
        ScreenBuffer(Vector2 screenSize);
        ~ScreenBuffer();

        void Clear();   // 콘솔 화면 초기화
        void Draw(const CHAR_INFO* const info); // 전달된 이미지(문자 2차원 배열)을 콘솔에 그리는 함수
        
        inline HANDLE GetHandle() const { return _handle; }

    private:
        HANDLE _handle;   // 콘솔 화면 버퍼 핸들
        COORD _size;
    };
}