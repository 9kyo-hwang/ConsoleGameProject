#pragma once
#include <Z1Shared/Protocol.h>
#include <Z1Shared/Serialization.h>

namespace Z1::Protocol
{
    inline constexpr std::size_t MaxBufferedRecvBytes = (std::size_t)MaxPacketSize << 1;

    struct Packet
    {
        PacketHeader header;
        std::vector<Byte> payload;
    };

    /// <summary>
    /// byte 누적, header 파싱, packet 크기 검증, 완성된 packet 분리
    /// </summary>
    class PacketFramer
    {
    public:
        enum class PopResult
        {
            Ready,
            NeedMoreData,
            Invalid,
        };

        // 누적 상한을 초과했는가
        bool Append(std::span<const Byte> bytes)
        {
            // 소비된 앞부분 먼저 compact
            CompactConsumedBytes();

            // 더 이상 수신 버퍼에 데이터 누적할 수 없음
            if (bytes.size() > MaxBufferedRecvBytes - _buffer.size())
            {
                return false;
            }

            _buffer.insert(_buffer.end(), bytes.begin(), bytes.end());
            return true;
        }

        PopResult TryPop(Packet& out)
        {
            // 1. header보다 짧으면 NeedMoreData
            const std::size_t bufferedSize = BufferedSize();
            if (bufferedSize < PacketHeaderSize)
            {
                return PopResult::NeedMoreData;
            }

            const std::span<const Byte> bufferedBytes(_buffer.data() + _readOffset, bufferedSize);

            PacketHeader header;
            PacketReader reader(bufferedBytes);

            // 2. 헤더 파싱 불가
            if (!reader.ReadU16(header.size) || !reader.ReadU16(header.type))
            {
                return PopResult::Invalid;
            }

            // 3. 헤더에 기록된 크기가 헤더 크기도 안되거나 최대 크기를 초과하면
            if (header.size < PacketHeaderSize || header.size > MaxPacketSize)
            {
                return PopResult::Invalid;
            }

            // 3. 아직 페이로드 파싱할 크기만큼 데이터가 없음
            if (bufferedSize < header.size)
            {
                return PopResult::NeedMoreData;
            }

            Packet packet;
            packet.header = header;

            auto payloadBegin = _buffer.begin() + _readOffset + PacketHeaderSize;
            auto payloadEnd = _buffer.begin() + _readOffset + header.size;
            packet.payload.assign(payloadBegin, payloadEnd);

            out = std::move(packet);
            _readOffset += header.size;

            return PopResult::Ready;
        }

        std::size_t BufferedSize() const noexcept
        {
            return _buffer.size() - _readOffset;
        }

    private:
        void CompactConsumedBytes()
        {
            if (_readOffset == 0) return;
            
            if (_readOffset == _buffer.size())
            {
                _buffer.clear();
            }
            else
            {
                _buffer.erase(_buffer.begin(), _buffer.begin() + _readOffset);
            }
            
            _readOffset = 0;
        }

    private:
        std::vector<Byte> _buffer;
        std::size_t _readOffset = 0;
    };
}