#include "Runtime.h"
#include <WinSock2.h>

namespace Net
{
    Runtime::Runtime() noexcept
    {
        WSADATA data{};
        const WORD version = MAKEWORD(2, 2);

        _errorCode = ::WSAStartup(version, &data);
        if (_errorCode != 0) return;

        if (data.wVersion != version)
        {
            ::WSACleanup();

            _errorCode = WSAVERNOTSUPPORTED;
            return;
        }

        _isValid = true;
    }

    Runtime::~Runtime() noexcept
    {
        if (_isValid)
        {
            ::WSACleanup();
        }
    }
}