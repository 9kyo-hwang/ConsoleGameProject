#include "pch.h"
#include "Session.h"

using namespace Z1::Protocol;

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
    if (result == 0 || WSA_IO_PENDING == ::WSAGetLastError())
    {
        // IOCP에 연결된 Socket Completion 대기(GetQueuedCompletionStatus에서 처리)
        // 혹은 정상적인 비동기 대기
        _recvPending = true;
        return true;
    }

    return false;
}

bool Session::HandleRecv(DWORD bytesTransferred)
{
    if (bytesTransferred == 0 || bytesTransferred > _recvBuffer.size())
    {
        return false;
    }

    const std::span<const Byte> recvdBytes(_recvBuffer.data(), bytesTransferred);
    if (!_framer.Append(recvdBytes))
    {
        return false;
    }

    while (true)
    {
        Packet packet;
        switch (_framer.TryPop(packet))
        {
        case PacketFramer::PopResult::Ready: _recvdPackets.push_back(std::move(packet)); break;
        case PacketFramer::PopResult::NeedMoreData: return true;
        case PacketFramer::PopResult::Invalid: return false;
        }
    }

    return true;
}

bool Session::TryPopRecvdPacket(Packet& outPacket)
{
    if (_recvdPackets.empty())
    {
        return false;
    }

    outPacket = std::move(_recvdPackets.front());
    _recvdPackets.pop_front();
    return true;
}

/// <summary>
/// SendQueue에 완성된 패킷을 Push
/// </summary>
bool Session::Send(std::vector<Z1::Protocol::Byte>&& packet)
{
    if (!_socket.IsValid() || packet.empty())
    {
        return false;
    }

    // WorldSnapshot 패킷이 매 Tick마다 뿌려지기 때문에,
    // 읽지 않는 클라의 전송 큐에 무한히 쌓이지 않도록 방어
    // Send에서 false가 반환되면 CloseSession -> 3.2초 이상 Send가 밀리면 세션 정리
    if (_sendQueue.size() >= MaxQueuedSendPackets)
    {
        return false;
    }

    _sendQueue.push_back(std::move(packet));

    // WSASend가 이미 진행 중이라면 Queue에 Push만 하고 빠져나옴
    if (_sendPending)
    {
        return true;
    }

    // Queue의 맨 앞 패킷 전송 시도
    return PostSend();
}

/// <summary>
/// ioThread가 CP로부터 완료 통지를 꺼냈는데 Send 이벤트인 경우 진입
/// </summary>
/// <param name="bytesTransferred"></param>
/// <returns></returns>
bool Session::HandleSend(DWORD bytesTransferred)
{
    // 해당 패킷이 처리됐거나(실제 pop이 여기서 이뤄지므로)
    if (_sendQueue.empty())
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

    // 전송한 Front 패킷이 부분적으로만 전송된 경우
    // Pop하지 않고 offset만 증가시킨 뒤 잔여 부분 다시 WSASend 시도
    if (_sendOffset < packet.size())
    {
        return PostSend();
    }

    // Front 패킷 전체 전송이 완료되어 pop + offset = 0
    _sendQueue.pop_front();
    _sendOffset = 0;

    // 아직 보낼 패킷이 남아있으면 이어서 전송
    if (!_sendQueue.empty())
    {
        return PostSend();
    }

    // 보낼 패킷이 더 이상 없다면 pending = false로 두고 대기
    return true;
}

/// <summary>
/// SendQueue의 맨 앞 패킷의 [아직 전송되지 않은 구간]을 WSABUF로 만들어 WSASend 호출
/// </summary>
/// <returns></returns>
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

    // 성공 또는 Pending: 현재 전송한 Front 패킷은 OS의 비동기 Send 완료 통지를 기다림
    if (result == 0 || ::WSAGetLastError() == WSA_IO_PENDING)
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
