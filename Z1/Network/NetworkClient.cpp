#include "pch.h"
#include "NetworkClient.h"
#include <array>
#include <Z1Shared/PacketCodec.h>

using namespace Z1::Protocol;
using namespace Net;

namespace
{
    /*
    * typedef struct fd_set {
    *   u_int fd_count;  // how many are SET?
    *   SOCKET  fd_array[64];  // an array of SOCKETs 
    * } fd_set;
    */

    struct FileDescriptors : fd_set
    {
        FileDescriptors() { Reset(); }

        void Add(SOCKET socket)
        {
            FD_SET(socket, this);
        }

        void Remove(SOCKET socket)
        {
            FD_CLR(socket, this);
        }

        bool Contains(SOCKET socket) const noexcept
        {
            return FD_ISSET(socket, this);
        }

        void Reset()
        {
            FD_ZERO(this);
        }

        bool IsEmpty() const noexcept
        {
            return fd_count == 0;
        }

        u_int Count() const noexcept
        {
            return fd_count;
        }
    };
}

NetworkClient::NetworkClient()
{
}

NetworkClient::~NetworkClient()
{
    Stop();
}

// TCP 연결 및 thread 시작
bool NetworkClient::Start(const Net::Endpoint& endpoint)
{
    if (!_runtime.IsValid() || _connected)
    {
        return false;
    }

    _socket = Socket::CreateTcp();
    if (!_socket.IsValid() || !_socket.Connect(endpoint))
    {
        _socket.Close();
        return false;
    }

    // Nagle OFF && non-blocking
    if (!_socket.SetNoDelay(true) || !_socket.SetNonBlocking(true))
    {
        _socket.Close();
        return false;
    }

    // C2S_Enter packet 생성: 서버 접속 성공 시(S2C_Enter) id가 발급됨
    std::vector<Byte> packet;
    if (!BuildPacket_C2SEnter(packet))
    {
        return false;
    }

    if (!QueuePacket(std::move(packet)))
    {
        _socket.Close();
        return false;
    }

    _stopRequested.exchange(false);
    _connected.exchange(true);
    _thread = std::thread(&NetworkClient::NetworkLoop, this);
    
    return true;
}

void NetworkClient::Stop()
{
    _stopRequested.store(true); // 이미 stop 됐어도, thread가 살아있다면 join해야 해서 return하지 말자.

    //_socket.Close();  // Start, Stop은 메인 스레드가 실행해서 여기서 닫으면 안됨!

    if (_thread.joinable())
    {
        _thread.join();
    }
}

// MainThread, NetworkThread 둘 다 읽어야 해서 Atomic
bool NetworkClient::IsConnected() const noexcept
{
    return _connected.load();
}

/// <summary>
/// 이미 파싱된 IncomingMessage를 꺼냄
/// _incomingMessage 큐를 건드리기 때문에 락을 잡자.
/// </summary>
/// <param name="message"></param>
/// <returns></returns>
bool NetworkClient::TryPopIncomingMessage(IncomingMessage& message)
{
    std::lock_guard lock(_recvMutex);

    if (_recvMessageQueue.empty())
    {
        return false;
    }

    message = std::move(_recvMessageQueue.front());
    _recvMessageQueue.pop_front();
    return true;
}

bool NetworkClient::QueuePacket(std::vector<Z1::Protocol::Byte>&& packet)
{
    if (packet.empty())
    {
        return false;
    }

    std::lock_guard lock(_sendMutex);
    if (_sendQueue.size() >= MaxQueuedPackets)
    {
        return false;
    }

    _sendQueue.push_back(std::move(packet));
    return true;
}

/// <summary>
/// Loop 종료 시 소켓 종료 + connected = false
/// 
/// Select 모델: Multiplexing I/O(소켓 리스트 등록 - I/O 가능한 소켓 알림)
/// RecvBuffer에 데이터가 없는데 Read하거나 | SendBuffer가 꽉 찼는데 Write하는 상황을 방지
/// </summary>
void NetworkClient::NetworkLoop()
{
    while (!_stopRequested)
    {
        // 서버로 보낼 패킷들 로컬 큐로 이동
        // NetworkThread가 _pendingPackets를 독점적으로 다루기 위해
        std::deque<std::vector<Byte>> packets;
        {
            std::lock_guard lock(_sendMutex);
            packets.swap((_sendQueue));
        }

        while (!packets.empty())
        {
            _pendingSendPackets.push_back(std::move(packets.front()));
            packets.pop_front();
        }

        FileDescriptors readSet;
        FileDescriptors writeSet;

        readSet.Add(_socket.GetNativeHandle());
        if (!_pendingSendPackets.empty())
        {
            writeSet.Add(_socket.GetNativeHandle());
        }

        TIMEVAL timeout
        {
            .tv_sec = 0,
            .tv_usec = 50'000
        };

        const int result = ::select(0, &readSet, writeSet.IsEmpty() ? nullptr : &writeSet, nullptr, &timeout);
        if (result == SOCKET_ERROR)
        {
            break;
        }

        if(readSet.Contains(_socket.GetNativeHandle()))
        {
            if (!TryRecvPacketFromServer())
            {
                break;
            }
        }

        if (writeSet.Contains(_socket.GetNativeHandle()))
        {
            if (!SendPacketsToServer())
            {
                break;
            }
        }
    }

    _socket.Close();
    _connected.store(false);
}

bool NetworkClient::TryRecvPacketFromServer()
{
    std::array<Byte, 4096> buffer{};
    while (true)
    {
        std::int32_t bytesRead = 0;
        if (!_socket.Recv(buffer.data(), buffer.size(), bytesRead))
        {
            if (_socket.GetLastError() == WSAEWOULDBLOCK)
            {
                return true;
            }

            return false;
        }

        if (bytesRead == 0)
        {
            return false;   // 서버 연결 종료
        }

        const std::span<const Byte> recvdBytes(buffer.data(), bytesRead);
        if (!_framer.Append(recvdBytes))
        {
            return false;
        }

        if (!ProcessRecvdData())
        {
            return false;
        }
    }

    return false;
}

/// <summary>
/// GameThread가 넣은 패킷들을 꺼내서 Send 시도
/// </summary>
/// <returns></returns>
bool NetworkClient::SendPacketsToServer()
{
    while (!_pendingSendPackets.empty())
    {
        const std::vector<Byte>& packet = _pendingSendPackets.front();
        if (_sendOffset >= packet.size())
        {
            return false;
        }

        const std::size_t remaining = packet.size() - _sendOffset;

        std::int32_t bytesSent = 0;
        if (!_socket.Send(packet.data() + _sendOffset, (std::int32_t)remaining, bytesSent))
        {
            if (_socket.GetLastError() == WSAEWOULDBLOCK)
            {
                return true;
            }

            return false;
        }

        if (bytesSent <= 0)
        {
            return false;
        }

        _sendOffset += (std::size_t)bytesSent;
        if (_sendOffset < packet.size())
        {
            // 일부분만 전송됨. 다음 select에서 이어 보냄
            return true;
        }

        _pendingSendPackets.pop_front();
        _sendOffset = 0;
    }

    return true;
}

/// <summary>
/// Typed Message로 역직렬화해서 IncomingMessageQueue에 저장,
/// TryPopIncoming -> Game::PumpNetwork
/// </summary>
/// <returns></returns>
bool NetworkClient::ProcessRecvdData()
{
    while (true)
    {
        Packet packet;
        switch (_framer.TryPop(packet))
        {
        case PacketFramer::PopResult::Ready:
        {
            if (!HandleServerPacket((PacketType)packet.header.type, packet.payload))
            {
                return false;
            }
        }
        break;
        case PacketFramer::PopResult::NeedMoreData: return true;
        case PacketFramer::PopResult::Invalid: return false;
        }
    }

    return true;
}

bool NetworkClient::PushIncomingMessage(IncomingMessage&& message)
{
    std::lock_guard lock(_recvMutex);

    // 지금은 queue 꽉차면 실패
    // 나중에 Snapshot만 최신 상태로 합치는 최적화
    if (_recvMessageQueue.size() >= MaxIncomingMessages)
    {
        return false;
    }

    _recvMessageQueue.push_back(std::move(message));
    return true;
}

bool NetworkClient::HandleServerPacket(PacketType type, std::span<const Byte> payload)
{
    switch (type)
    {
    case PacketType::S2C_Enter: return HandleEnter(payload);
    case PacketType::S2C_WorldSnapshot: return HandleWorldSnapshot(payload);
    default: return false;
    }
}

bool NetworkClient::HandleEnter(std::span<const Byte> payload)
{
    if (payload.size() != sizeof(std::uint16_t) + sizeof(std::uint32_t))
    {
        return false;
    }

    std::uint32_t playerId = 0;
    if (!ParsePayload_S2CEnter(payload, playerId))
    {
        return false;
    }

    return PushIncomingMessage(EnterMessage{ playerId });
}

bool NetworkClient::HandleWorldSnapshot(std::span<const Byte> payload)
{
    WorldSnapshot snapshot;
    if (!ParsePayload_S2CWorldSnapshot(payload, snapshot)) 
    {
        return false;
    }

    return PushIncomingMessage(IncomingMessage{ std::move(snapshot) });
}
