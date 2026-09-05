#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // Windows.h의 불필요한 하위 헤더 안끌어오도록

#include <WinSock2.h>   // Windows.h보다 먼저 오도록

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
using byte = unsigned char;

#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>