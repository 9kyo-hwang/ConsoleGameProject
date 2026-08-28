#pragma once
#include "Export.h"
#include <WinSock2.h>
#include <cstdint>

namespace Net
{
    class Endpoint;

    class SOCKET_API Socket
    {
    public:
        Socket(SOCKET handle = INVALID_SOCKET);
        ~Socket();

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;

        static Socket CreateTcp();

        inline SOCKET GetNativeHandle() const { return _handle; }
        inline int GetLastError() const noexcept { return _error; }

        bool IsValid() const;
        bool SetNonBlocking(bool isNonBlocking);    // select 모델 사용 시 필요
        bool SetNoDelay(bool isNoDelay);

        Socket Accept(Endpoint* outAddr = nullptr);    // Listen 상태에서 새 클라이언트 접속 시, 그와 통신하는 socket 반환
        bool Bind(const Endpoint& addr);
        bool Connect(const Endpoint& endpoint);
        void Close();
        bool Listen(int maxBacklog = SOMAXCONN);       // Bind를 성공한 상태에서 client를 받을 상태로 전환
        bool Recv(void* data, std::int32_t bufferSize, std::int32_t& bytesRead);
        SOCKET ReleaseNativeSocket();
        bool Send(const void* data, std::int32_t count, std::int32_t& bytesSent);

    private:
        template<typename T>
        bool SetSocketOption(std::int32_t optionName, T optionValue, std::int32_t level = IPPROTO_TCP)
        {
            if (::setsockopt(_handle, level, optionName, (const char*)&optionValue, sizeof(T)) == 0)
            {
                return true;
            }

            _error = ::WSAGetLastError();
            return false;
        }

    private:
        SOCKET _handle;
        int _error = 0;
    };
}
