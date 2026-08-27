#include "pch.h"
#include "Session.h"

Session::Session(Net::Socket&& socket) noexcept
    : _socket(std::move(socket))
    , _recvBufferView{.len = (ULONG)_recvBuffer.size(), .buf = (char*)_recvBuffer.data()}
{

}

bool Session::PostRecv()
{
    if (!_socket.IsValid()) return false;

    // 이전 수신 completion이 끝난 뒤 호출
    std::memset(&_recvOverlapped, 0, sizeof(_recvOverlapped));
    _recvFlags = 0;

    DWORD bytesRecvd = 0;
    const int result = ::WSARecv(_socket.GetNativeHandle(), &_recvBufferView, 1, &bytesRecvd, &_recvFlags, &_recvOverlapped, nullptr);
    if (result == 0)
    {
        // IOCP에 연결된 Socket Completion 대기(GetQueuedCompletionStatus에서 처리)
        return true;
    }

    const int error = ::WSAGetLastError();
    if (error == WSA_IO_PENDING)
    {
        // 정상적인 비동기 대기
        return true;
    }

    return false;
}

bool Session::HandleRecv(DWORD transferredBytes)
{
    if (transferredBytes == 0 || transferredBytes > _recvBuffer.size())
    {
        return false;
    }

    _recvdData.insert(_recvdData.end(), _recvBuffer.begin(), _recvBuffer.begin() + transferredBytes);

    std::size_t consumed = 0;
    while (true)
    {
        // 헤더도 못읽으면 break
        const std::size_t available = _recvdData.size() - consumed;
        if (available < PacketHeaderSize)
        {
            break;
        }

        std::span<const Byte> bytes(_recvdData.data() + consumed, available);

        std::size_t offset = 0;
        std::uint16_t packetSize = 0, rawType = 0;
        if (!ReadU16(bytes, offset, packetSize) || !ReadU16(bytes, offset, rawType))
        {
            return false;
        }

        // 전체 패킷 크기가 헤더보다 작거나 최대 크기를 벗어나면 안됨
        if (packetSize < PacketHeaderSize || packetSize > MaxPacketSize)
        {
            return false;
        }

        // 수신 데이터 크기가 패킷 크기보다 작으면
        if (available < packetSize)
        {
            break;
        }

        RecvdPacket packet;
        packet.rawType = rawType;

        auto payloadBegin = _recvdData.begin() + consumed + PacketHeaderSize;
        auto payloadEnd = _recvdData.begin() + consumed + packetSize;

        packet.payload.assign(payloadBegin, payloadEnd);
        _recvdPackets.push_back(std::move(packet));

        consumed += packetSize;
    }

    if (consumed > 0)
    {
        _recvdData.erase(_recvdData.begin(), _recvdData.begin() + consumed);
    }

    return true;
}

bool Session::TryPopRecvdPacket(RecvdPacket& outPacket)
{
    if (_recvdPackets.empty())
    {
        return false;
    }

    outPacket = std::move(_recvdPackets.front());
    _recvdPackets.pop_front();
    return true;
}
