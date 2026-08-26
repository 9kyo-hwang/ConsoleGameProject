#pragma once
#include "Export.h"
#include <cstdint>
#include <string_view>
#include <WinSock2.h>

/// <summary>
/// IP 주소와 Port를 저장하는 값 객체
/// Socket Handle 소유 X
/// sockaddr_in을 노출하지 않도록.
/// </summary>
namespace Net
{
    class SOCKET_API Endpoint
    {
    public:
        Endpoint() noexcept = default;

        static Endpoint Any(std::uint16_t port) noexcept;
        static Endpoint Loopback(std::uint16_t port) noexcept;
        static bool TryParseIPv4(std::string_view address, std::uint16_t port, Endpoint& result);

        inline bool IsValid() const noexcept { return _isValid; }
        inline std::uint16_t GetPort() const noexcept { return IsValid() ? ntohs(_address.sin_port) : 0; }

    private:
        explicit Endpoint(const SOCKADDR_IN& address) noexcept
            : _address(address)
            , _isValid(true)
        {

        }

        friend class Socket;

        SOCKADDR_IN _address{};
        bool _isValid = false;
    };
}
