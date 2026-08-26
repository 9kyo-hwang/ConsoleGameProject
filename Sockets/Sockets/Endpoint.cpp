#include "Endpoint.h"
#include <WS2tcpip.h>
#include <cstring>

namespace Net
{
    Endpoint Endpoint::Any(std::uint16_t port) noexcept
    {
        Endpoint result;
        result._address.sin_family = AF_INET;
        result._address.sin_addr.s_addr = htonl(INADDR_ANY);
        result._address.sin_port = htons(port);
        result._isValid = true;

        return result;
    }

    Endpoint Endpoint::Loopback(std::uint16_t port) noexcept
    {
        Endpoint result;
        if (TryParseIPv4("127.0.0.1", port, result))
        {
            return result;
        }

        return Endpoint();
    }

    bool Endpoint::TryParseIPv4(std::string_view address, std::uint16_t port, Endpoint& result)
    {
        result = Endpoint();

        if (address.empty() || address.size() >= INET_ADDRSTRLEN)
        {
            return false;
        }

        char buffer[INET_ADDRSTRLEN]{};
        std::memcpy(buffer, address.data(), address.size());
        buffer[address.size()] = '\0';

        SOCKADDR_IN native{};
        native.sin_family = AF_INET;
        native.sin_port = htons(port);

        if (::inet_pton(AF_INET, buffer, &native.sin_addr) != 1)
        {
            return false;
        }

        result._address = native;
        result._isValid = true;

        return true;
    }
}