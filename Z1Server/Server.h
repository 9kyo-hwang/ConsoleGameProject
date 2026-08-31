#pragma once
#include <cstdint>
#include <Sockets/Socket.h>
#include <CompletionPort.h>

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <Session.h>
#include <OverworldSimulation.h>
#include <deque>
#include <mutex>

namespace Z1::Protocol
{
    struct Packet;
}

class Server
{
public:
    Server() = default;

    bool Start(std::uint16_t port);
    void WaitForShutdown();
    void Stop();

    bool HandleClientPacket(Session& session, const Z1::Protocol::Packet& packet);
    bool HandleEnter(Session& session, std::span<const Z1::Protocol::Byte> payload);
    bool HandleInput(Session& session, std::span<const Z1::Protocol::Byte> payload);

private:
    // AcceptLoop에서 _sessions.push_back, IOLoop에서 _sessions 순회
    // 다른 두 스레드가 동일한 컨테이너에 접근 중이라, 
    // - Accept은 accept만 하도록
    // - 세션 생성, IOCP 연결, _session 등록은 모두 IOLoop에서 하도록 변경
    void AcceptLoop();
    void ProcessAcceptedSockets();

    void IOLoop();
    bool RegisterAcceptedSocket(Net::Socket&& socket);
    
    void CloseSession(Session& session);

private:
    void Tick();
    void BroadcastWorldSnapshot();

private:
    Net::Socket _listener;
    CompletionPort _cp;

    std::thread _acceptThread;
    std::thread _ioThread;

    std::atomic_bool _stopRequested = false;

    std::mutex _acceptMutex;
    std::deque<Net::Socket> _acceptedSockets;
    std::vector<std::unique_ptr<Session>> _sessions;    // only IOLoop

private:    // IO Thread만 접근한다는 전제
    std::uint32_t _playerId = 1;
    OverworldSimulation _overworld;

    std::uint32_t _serverTick = 0;  // 번호는 무슨 의미일까
};
