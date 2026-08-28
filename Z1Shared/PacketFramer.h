#pragma once
#include <Z1Shared/Protocol.h>

namespace Z1::Protocol
{
    inline constexpr std::size_t MaxBufferedRecvdBytes = (std::size_t)MaxPacketSize << 1;

    struct Packet
    {
        PacketHeader header;
        std::vector<Byte> payload;
    };

    enum class PacketResult
    {
        Ready,
        NeedMoreData,
        Invalid,
    };

    /// <summary>
    /// byte 누적, header 파싱, packet 크기 검증, 완성된 packet 분리, 누적량 상한
    /// </summary>
    class PacketFramer
    {
    public:
        bool Append(std::span<const Byte> bytes)
        {

        }

        bool TryPop(FramedPacket& packet)
        {

        }

    private:
        std::vector<Byte> _buffer;
        std::size_t _readOffset = 0;
    };
}