#pragma once
#include <Sockets/Socket.h>
#include <array>
#include <vector>
#include <Z1Shared/Protocol.h>
#include <deque>
#include <optional>
#include <Z1Shared/PacketFramer.h>

class Session
{
public:
    explicit Session(Net::Socket&& socket) noexcept;

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    inline SOCKET GetNativeHandle() const noexcept { return _socket.GetNativeHandle(); }
    inline OVERLAPPED* GetRecvOverlapped() noexcept { return &_recvOverlapped; }
    inline OVERLAPPED* GetSendOverlapped() noexcept { return &_sendOverlapped; }

    // Pending bool 변수를 풀어주는 역할.
    inline bool HasIoPending() const noexcept { return _recvPending || _sendPending; }
    inline void AckRecvCompletion() noexcept { _recvPending = false; }
    inline void AckSendCompletion() noexcept { _sendPending = false; }

    // WSARecv 완료된 byte 수신 -> TCP 누적 -> 헤더 크기 검증 -> 완성된 패킷 분리 -> Server가 꺼낼 수 있도록 보관
    bool PostRecv();
    bool HandleRecv(DWORD bytesTransferred);
    bool TryPopRecvdPacket(Z1::Protocol::Packet& outPacket);

    bool Send(std::vector<Z1::Protocol::Byte>&& packet);    // 이미 framing된, "완성된" 패킷만 받음
    bool HandleSend(DWORD bytesTransferred);

    inline bool IsClosing() const noexcept { return _closing; }
    inline void SetClosing() noexcept { _closing = true; }
    inline bool CanDestroy() const noexcept { return _closing && !HasIoPending(); }
    void Close() { _socket.Close(); }

private:
    bool PostSend();

public:
    inline std::optional<std::uint32_t> GetPlayerId() const noexcept { return _playerId; }
    inline bool IsEntered() const noexcept { return _playerId.has_value(); }
    bool Enter(std::uint32_t playerId);

private:
    Net::Socket _socket;
    bool _closing = false;

    // 수신 완료 전까지 살아있어야 해서 지역 변수 X
    OVERLAPPED _recvOverlapped{};           // IOCP 완료 결과를 식별하는 구조체
    WSABUF _recvBufferView{};               // recvBuffer를 WSABUF 형식으로 설정
    DWORD _recvFlags = 0;

    std::array<Z1::Protocol::Byte, 4096> _recvBuffer{}; // WSARecv가 직접 채우는 임시 버퍼
    Z1::Protocol::PacketFramer _framer;
    std::deque<Z1::Protocol::Packet> _recvdPackets;              // 수신 큐
    bool _recvPending = false;

    inline static constexpr std::size_t MaxQueuedSendPackets = 64;
    OVERLAPPED _sendOverlapped{};
    WSABUF _sendBufferView{};
    
    // 현재 IOLoop 하나만 세션 Send를 호출해 mutex는 필요 없음. 이후 멀티스레드 도입 시 정책 설정
    std::deque<std::vector<Z1::Protocol::Byte>> _sendQueue; // front: 전송 중인 완성 패킷. IO에 넘긴 메모리 수명 보장 컨테이너 -> Close에서 비우면 안됨
    std::size_t _sendOffset = 0;    // 완성 패킷 중 이미 전송 완료된 바이트 수
    bool _sendPending = false;      // WSASend completion을 기다리는지 여부

private:
    std::optional<std::uint32_t> _playerId;
};

