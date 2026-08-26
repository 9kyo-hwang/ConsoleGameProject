#include "pch.h"
#include "Session.h"

Session::Session(Net::Socket&& socket) noexcept
    : _socket(std::move(socket))
    , _recvBufferView{.len = (ULONG)_recvBuffer.size(), .buf = _recvBuffer.data()}
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
    return true;
}
