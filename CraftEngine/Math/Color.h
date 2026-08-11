#pragma once

#include <Core/Core.h>
#include <Windows.h>

namespace Craft
{
    enum class CRAFT_API Color : WORD
    {
        Blue = FOREGROUND_BLUE,
        Green = FOREGROUND_GREEN,
        Cyan = Green | Blue,
        Red = FOREGROUND_RED,
        Magenta = Red | Blue,
        Yellow = Red | Green,
        White = Blue | Green | Red,
        BrightRed = Red | FOREGROUND_INTENSITY,
        BrightYellow = Yellow | FOREGROUND_INTENSITY,
        BrightGreen = Green | FOREGROUND_INTENSITY,
        BrightWhite = White | FOREGROUND_INTENSITY
    };
}