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

// 패킷을 queue에 넣고, send pending이 아니면 WSASend
bool Session::Send(std::vector<Z1::Protocol::Byte>&& packet)
{
    if (!_socket.IsValid() || packet.empty())
    {
        return false;
    }

    _sendQueue.push_back(std::move(packet));

    if (_sendPending)
    {
        return true;
    }

    return PostSend();
}

// 클라가 Send 이벤트를 완료했을 때
bool Session::HandleSend(DWORD bytesTransferred)
{
    if (!_sendPending || _sendQueue.empty())
    {
        return false;
    }

    const std::vector<Byte>& packet = _sendQueue.front();
    const size_t remainingSize = packet.size() - _sendOffset;

    if (bytesTransferred == 0 || bytesTransferred > remainingSize)
    {
        return false;
    }

    _sendOffset += bytesTransferred;
    _sendPending = false;

    // 아직 해당 패킷을 다 전송 못했으면 나머지 재등록
    if (_sendOffset < packet.size())
    {
        return PostSend();
    }

    // 이번 패킷 전송 완료
    _sendQueue.pop_front();
    _sendOffset = 0;

    // 아직 보낼 패킷이 남아있으면 이어서 전송
    if (!_sendQueue.empty())
    {
        return PostSend();
    }

    return true;
}

// 전송 큐 맨 앞 패킷에서 아직 못보낸 구간만 WSASend 등록
bool Session::PostSend()
{
    if (!_socket.IsValid() || _sendQueue.empty())
    {
        return false;
    }

    const std::vector<Byte>& packet = _sendQueue.front();
    if (_sendOffset >= packet.size())   // 이미 보낸 패킷이라 커서가 패킷 뒤에 있다면
    {
        return false;
    }

    const size_t remainingSize = packet.size() - _sendOffset;
    std::memset(&_sendOverlapped, 0, sizeof(_sendOverlapped));

    _sendBufferView = { .len = (ULONG)remainingSize, .buf = (char*)(packet.data() + _sendOffset) };

    DWORD bytesSent = 0;
    const int result = ::WSASend(_socket.GetNativeHandle(), &_sendBufferView, 1, &bytesSent, 0, &_sendOverlapped, nullptr);

    // 성공했더라도 Completion 기준으로 완료 처리 해야하므로, 전송 큐 pop 하면 안됨!
    if (result == 0 || _socket.GetLastError() == WSA_IO_PENDING)
    {
        _sendPending = true;
        return true;
    }

    return false;
}

bool Session::Enter(std::uint32_t playerId)
{
    if (_playerId.has_value())
    {
        return false;
    }

    _playerId = playerId;
    return true;
}
