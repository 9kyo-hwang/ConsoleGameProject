#include "Socket.h"
#include <utility>
#include "Endpoint.h"

namespace Net
{
    Socket::Socket(SOCKET handle)
        : _handle(handle)
    {
    }

    Socket::~Socket()
    {
        Close();
    }

    Socket::Socket(Socket&& other) noexcept
        : _handle(std::exchange(other._handle, INVALID_SOCKET))
    {

    }

    Socket& Socket::operator=(Socket&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            _handle = std::exchange(other._handle, INVALID_SOCKET);
        }

        return *this;
    }

    Socket Socket::CreateTcp()
    {
        return Socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    }

    bool Socket::IsValid() const
    {
        return _handle != INVALID_SOCKET;
    }

    bool Socket::SetNonBlocking(bool isNonBlocking)
    {
        if (!IsValid())
        {
            return false;
        }

        u_long mode = isNonBlocking ? 1ul : 0ul;
        if (::ioctlsocket(_handle, FIONBIO, &mode) == 0)
        {
            return true;
        }

        _error = ::WSAGetLastError();
        return false;
    }

    bool Socket::SetNoDelay(bool isNoDelay)
    {
        if (!IsValid())
        {
            return false;
        }

        return SetSocketOption(TCP_NODELAY, isNoDelay ? TRUE : FALSE);
    }

    bool Socket::Bind(const Endpoint& addr)
    {
        if (!IsValid() || !addr.IsValid())
        {
            return false;
        }

        if (::bind(_handle, (const SOCKADDR*)&addr._address, sizeof(addr._address)) == 0)
        {
            return true;
        }

        _error = ::WSAGetLastError();
        return false;
    }

    bool Socket::Listen(int maxBacklog)
    {
        if (!IsValid())
        {
            return false;
        }

        if (::listen(_handle, maxBacklog) == 0)
        {
            return true;
        }

        _error = ::WSAGetLastError();
        return false;
    }

    // true + bytesRead > 0: 수신 성공
    // true + bytesRead == 0: 상대방 연결 정상 종료(bufferSize > 0인 경우)
    // false: GetLastError()
    bool Socket::Recv(void* data, std::int32_t bufferSize, std::int32_t& bytesRead)
    {
        bytesRead = ::recv(_handle, (char*)data, bufferSize, 0);
        if (bytesRead >= 0)
        {
            return true;
        }

        _error = ::WSAGetLastError();
        return false;
    }

    Socket Socket::Accept(Endpoint* outAddr)
    {
        if (!IsValid())
        {
            return Socket();
        }

        SOCKADDR_IN peeraddr{};
        int addrlen = sizeof(peeraddr);

        const SOCKET peerHandle = ::accept(_handle, (SOCKADDR*)&peeraddr, &addrlen);
        if (peerHandle == INVALID_SOCKET)
        {
            return Socket();
        }

        if (outAddr)
        {
            *outAddr = Endpoint(peeraddr);
        }

        return Socket(peerHandle);
    }

    bool Socket::Connect(const Endpoint& endpoint)
    {
        if (!IsValid() || !endpoint.IsValid())
        {
            return false;
        }

        if (::connect(_handle, (const SOCKADDR*)&endpoint._address, sizeof(endpoint._address)) == 0)
        {
            return true;
        }

        _error = ::WSAGetLastError();
        return false;
    }

    void Socket::Close()
    {
        if (_handle != INVALID_SOCKET)
        {
            closesocket(_handle);
            _handle = INVALID_SOCKET;
        }
    }

    SOCKET Socket::ReleaseNativeSocket()
    {
        SOCKET released = _handle;
        _handle = INVALID_SOCKET;
        return released;
    }

    // true + bytesSent > 0: 일부 또는 전체 전송 성공
    // false: GetLastError 확인(WSAEWOULDBLOCK이면 넌블록킹 소켓이 처리 불가한 상태. 에러가 아니라 다음 select. 그 외에는 에러)
    bool Socket::Send(const void* data, std::int32_t count, std::int32_t& bytesSent)
    {
        bytesSent = ::send(_handle, (const char*)data, count, 0);
        if (bytesSent > 0)
        {
            return true;
        }

        _error = ::WSAGetLastError();
        return false;
    }
}