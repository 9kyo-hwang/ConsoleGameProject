#pragma once
#include "Export.h"

/// <summary>
/// 정적 Startup / Cleanup의 RAII 객체화. 프로세스마다 하나.
/// Socket보다 먼저 생성, Socket보다 나중에 소멸
/// </summary>
namespace Net
{
    class SOCKET_API Runtime final
    {
    public:
        Runtime() noexcept;
        ~Runtime() noexcept;

        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;

        inline bool IsValid() const noexcept { return _isValid; }
        inline int GetErrorCode() const noexcept { return _errorCode; }

    private:
        bool _isValid = false;
        int _errorCode = 0;
    };
}
