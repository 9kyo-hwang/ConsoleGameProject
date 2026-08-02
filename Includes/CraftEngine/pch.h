#pragma once

#define FORCEINLINE __forceinline
#define NOMINMAX

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
using byte = unsigned char;

#include <Windows.h>

#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <string>
#include <array>