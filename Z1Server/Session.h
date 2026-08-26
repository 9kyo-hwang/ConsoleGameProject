#pragma once
#include <Sockets/Socket.h>
#include <array>
#include <vector>

/// <summary>
/// 우선은 Recv 먼저 처리하자.
/// Socket, Recv OVERLAPPED, recvbuffer, receivedBytes
/// </summary>
class Session
{
public:
    explicit Session(Net::Socket&& socket) noexcept;

    Session(const Net::Socket&) = delete;
    Session& operator=(const Net::Socket&) = delete;

    inline SOCKET GetNativeHandle() const noexcept { return _socket.GetNativeHandle(); }
    inline OVERLAPPED* GetRecvOverlapped() noexcept { return &_recvOverlapped; }

    bool PostRecv();
    bool HandleRecv(DWORD transferredBytes);

    void Close() { _socket.Close(); }

private:
    Net::Socket _socket;

    // 수신 완료 전까지 살아있어야 해서 지역 변수 X
    OVERLAPPED _recvOverlapped{};           // IOCP 완료 결과를 식별하는 구조체
    WSABUF _recvBufferView;                 // recvBuffer를 WSABUF 형식으로 설정
    DWORD _recvFlags = 0;

    std::array<char, 4096> _recvBuffer{};   // WSARecv가 직접 채우는 임시 버퍼
    std::vector<char> _recvdData;           // 수신 완료 후 누적하는 데이터. TCP Framing에 사용
};

