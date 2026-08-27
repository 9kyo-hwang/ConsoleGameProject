#pragma once
#include <Sockets/Socket.h>
#include <array>
#include <vector>
#include <Z1Shared/Protocol.h>
#include <deque>

class Session
{
public:
    struct RecvdPacket
    {
        std::uint16_t rawType = 0;  // no PacketType -> server의 switch에서 해석할 예정
        std::vector<Z1::Protocol::Byte> payload;
    };

    explicit Session(Net::Socket&& socket) noexcept;

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    inline SOCKET GetNativeHandle() const noexcept { return _socket.GetNativeHandle(); }
    inline OVERLAPPED* GetRecvOverlapped() noexcept { return &_recvOverlapped; }

    /// <summary>
    /// WSARecv 완료된 byte 수신 -> TCP 누적 -> 헤더 크기 검증 -> 완성된 패킷 분리 -> Server가 꺼낼 수 있도록 보관
    /// </summary>
    bool PostRecv();
    bool HandleRecv(DWORD transferredBytes);
    bool TryPopRecvdPacket(RecvdPacket& outPacket);

    void Close() { _socket.Close(); }

private:
    Net::Socket _socket;

    // 수신 완료 전까지 살아있어야 해서 지역 변수 X
    OVERLAPPED _recvOverlapped{};           // IOCP 완료 결과를 식별하는 구조체
    WSABUF _recvBufferView;                 // recvBuffer를 WSABUF 형식으로 설정
    DWORD _recvFlags = 0;

    std::array<Z1::Protocol::Byte, 4096> _recvBuffer{}; // WSARecv가 직접 채우는 임시 버퍼
    std::vector<Z1::Protocol::Byte> _recvdData;         // 수신 완료 후 누적하는 데이터. TCP Framing에 사용
    std::deque<RecvdPacket> _recvdPackets;              // 수신 큐
};

