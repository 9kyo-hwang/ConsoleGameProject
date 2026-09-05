#pragma once

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

#if defined(SOCKET_BUILD_DLL)
#define SOCKET_API DLLEXPORT
#else
#define SOCKET_API DLLIMPORT
#endif