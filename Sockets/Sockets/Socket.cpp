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

    Socket Socket::Accept(Endpoint* outAddr)
    {
        if (!IsValid())
        {
            return Socket();
        }

        SOCKADDR_IN clientaddr{};
        int addrlen = sizeof(clientaddr);

        const SOCKET clientHandle = ::accept(_handle, (SOCKADDR*)&clientaddr, &addrlen);
        if (clientHandle == INVALID_SOCKET)
        {
            return Socket();
        }

        if (outAddr)
        {
            *outAddr = Endpoint(clientaddr);
        }

        return Socket(clientHandle);
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

    SOCKET Socket::Release()
    {
        SOCKET released = _handle;
        _handle = INVALID_SOCKET;
        return released;
    }
}