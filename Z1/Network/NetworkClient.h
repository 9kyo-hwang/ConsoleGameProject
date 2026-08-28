#pragma once
#include <Z1Shared/Protocol.h>
#include <Z1Shared/Serialization.h>
#include <Sockets/Endpoint.h>
#include <Sockets/Runtime.h>
#include <Sockets/Socket.h>
#include <cstdint>
#include <vector>
#include <variant>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <cstddef>
#include <span>

struct EnterMessage
{
    std::uint32_t playerId = 0;
};

struct WorldSnapshot
{
    std::uint32_t serverTick = 0;   // 확인용
    std::vector<Z1::Protocol::SnapshotPlayerState> players;
};

using IncomingMessage = std::variant<EnterMessage, WorldSnapshot>;

/// <summary>
/// Client -> Server 하나만 붙는 구조이고, 현재 구현 단계 상 입력 소켓만 전송하는 구조라
/// Select() 모델 + 송수신 담당 스레드 하나로 심플하게 구조를 잡음
/// </summary>
class NetworkClient
{
public:
    NetworkClient();
    ~NetworkClient();

    bool Start(const Net::Endpoint& endpoint);  // 메인 스레드에서 호출!
    void Stop();

    bool IsConnected() const noexcept;
    bool TryPopIncomingMessage(IncomingMessage& message);

    inline int GetLastError() const noexcept { return _socket.GetLastError(); }

private:
    bool QueuePacket(std::vector<Z1::Protocol::Byte>&& packet); // 패킷을 outgoingPacketQueue에 밀어넣는 헬퍼
    void NetworkLoop();
    bool TryRecvPacketFromServer(); // nonblocking recv()를 WSAEWOULDBLOCK이 나올 때까지 반복
    bool SendPacketsToServer();
    bool ProcessRecvdData();
    bool PushIncomingMessage(IncomingMessage&& message);

private:
    bool HandleServerPacket(Z1::Protocol::PacketType type, std::span<const Z1::Protocol::Byte> payload);
    bool HandleEnter(std::span<const Z1::Protocol::Byte> payload);
    bool HandleWorldSnapshot(std::span<const Z1::Protocol::Byte> payload);

private:
    // 서버 세션의 SendQueue도 64개, 최대 패킷 크기 4096byte라 큐 1개는 약 256KB로 제한
    // 현재 서버가 20Hz 틱이라 64개 패킷이 누적되면 최대 3.2초 동안 쌓인 것
    // 클라에서 3.2초 지연은 엔진이 멈췄거나 네트워크가 끊겼거나로 판단할 수도 있을 듯
    inline static constexpr std::size_t MaxQueuedPackets = 64;
    inline static constexpr std::size_t MaxIncomingMessages = 64;

    Net::Runtime _runtime;
    Net::Socket _socket;
    std::thread _thread;

    std::atomic_bool _stopRequested = false;
    std::atomic_bool _connected = false;
    
    // GameThread -> NetworkThread: 서버로 보낼
    std::mutex _sendMutex;
    std::deque<std::vector<Z1::Protocol::Byte>> _sendQueue; // 이동 input은 최신 방향으로 합치는 식으로 개선 가능할 듯?

    // Only NetworkThread
    std::deque<std::vector<Z1::Protocol::Byte>> _pendingSendPackets;
    std::size_t _sendOffset = 0;
    std::vector<Z1::Protocol::Byte> _recvdData;

    // NetworkThread -> GameThread: 서버로부터 받은 걸 넘겨주는 역할
    std::mutex _recvMutex;
    std::deque<IncomingMessage> _recvMessageQueue;  // 이전에 미처리한 snapshot 버리고 최신 것만 남기도록?
};

