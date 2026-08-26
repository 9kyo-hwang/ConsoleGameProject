#pragma once
#include "Export.h"
#include <WinSock2.h>

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
        
        bool Bind(const Endpoint& addr);
        bool Listen(int maxBacklog = SOMAXCONN);       // Bind를 성공한 상태에서 client를 받을 상태로 전환
        Socket Accept(Endpoint* outAddr = nullptr);    // Listen 상태에서 새 클라이언트 접속 시, 그와 통신하는 socket 반환
        bool Connect(const Endpoint& endpoint);
        void Close();
        SOCKET Release();

    private:
        SOCKET _handle;
        int _error = 0;
    };
}
