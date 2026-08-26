#pragma once
#include <cstdint>
#include <Sockets/Socket.h>

#include <Windows.h>    // WinSock2 헤더보다 먼저 포함되면 안됨!
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <Session.h>

class Server
{
public:
    Server() = default;

    bool Start(std::uint16_t port);
    void WaitForShutdown();
    void Stop();

private:
    void AcceptLoop();  // 별도 스레드로 처리하기 위함(accept: blocking)
    void IOLoop();

private:
    Net::Socket _listener;
    HANDLE _completionPort = nullptr;

    std::thread _acceptThread;
    std::thread _ioThread;

    std::atomic_bool _stopRequested = false;

    std::vector<std::unique_ptr<Session>> _sessions;
};
