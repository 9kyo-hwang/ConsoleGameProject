#include "pch.h"
#include "Server.h"
#include <Sockets/Endpoint.h>
#include <chrono>
#include <Z1Shared/PacketFramer.h>
#include <Z1Shared/PacketCodec.h>
#include <string>

using namespace Net;
using namespace Z1::Protocol;

bool Server::Start(std::uint16_t port)
{
    std::string mapError;
    if (!_overworld.LoadBlockingMap("../Content/Z1/Maps/Overworld/BlockingMap.txt", mapError))
    {
        std::cerr << "Failed to load BlockingMaps: " << mapError << '\n';
        return false;
    }

    constexpr std::uint32_t ServerOverworldSeed = 0x5A314D50;
    if (!_overworld.SpawnEnemies(ServerOverworldSeed))
    {
        std::cerr << "Failed to spawn overworld enemies\n";
        return false;
    }

    _listener = Socket::CreateTcp();
    if (!_listener.IsValid())
    {
        return false;
    }

    if (!_listener.Bind(Endpoint::Any(port)))
    {
        return false;
    }

    if (!_listener.Listen())
    {
        return false;
    }

    if (!_cp.Create())
    {
        return false;
    }

    _acceptThread = std::thread(&Server::AcceptLoop, this);
    _ioThread = std::thread(&Server::IOLoop, this);
    return true;
}

void Server::WaitForShutdown()
{
    // Stop을 바로 호출하면 서버가 시작하자마자 종료됨
    // 얘는 기다리는 역할이기 때문에, 임시로 콘솔 서버의 특징을 살려 Enter 입력 대기
    std::cin.get();
    Stop();
}

void Server::Stop()
{
    // 이전에 이미 stop 요청된 상태라면
    if (_stopRequested.exchange(true))
    {
        return;
    }

    _listener.Close();

    // accept 전담 스레드 종료 가능하다면 종료
    if (_acceptThread.joinable())
    {
        _acceptThread.join();
    }

    // IOCP Worker 종료: overlapped == nullptr인 completion 종료 신호 처리
    _cp.PostShutdown();

    if (_ioThread.joinable())
    {
        _ioThread.join();
    }
}

// 수신 패킷의 타입을 보고 타입 별 핸들 함수를 호출하는 역할
bool Server::HandleClientPacket(Session& session, const Packet& packet)
{
    switch ((PacketType)packet.header.type)
    {
    case PacketType::C2S_Enter: return HandleEnter(session, packet.payload);
    case PacketType::C2S_Input: return HandleInput(session, packet.payload);
    default: return false;
    }
}

// C2S_Enter 패킷 처리 함수
bool Server::HandleEnter(Session& session, std::span<const Z1::Protocol::Byte> payload)
{
    // 1. payload 크기가 sizeof(uint16_t)인지
    // 2. protocol version이 ProtocolVersion인지
    // 이 검사는 내부적으로 사용하는 PacketReader에 의해(Read(), IsAtEnd()) 수행됨
    if (!ParsePayload_C2SEnter(payload))
    {
        return false;
    }

    if (!session.Enter(_playerId++))
    {
        return false;
    }

    // C2S_Enter 성공했으니 서버 월드에 입장시키자.
    const std::uint32_t playerId = *session.GetPlayerId();
    if (!_overworld.AddPlayer(playerId))
    {
        return false;
    }

    /*
    * S2C_Enter 패킷을 보내자.
    * 10바이트씩 끊어 보내면 아래 값이 확인됨
    * 00 0A 00 65 00 01 00 00 00 01
    * - 00 0A: 10 -> 패킷 전체 크기(헤더 4 + 페이로드 6)
    * - 00 65: 101 -> PacketType::S2C_Enter
    * - 00 01: 1 -> ProtocolVersion
    * - 00 00 00 01: 1 -> 서버가 세션에 할당한 플레이어 ID
    */

    std::vector<Byte> enterPacket;
    if (!BuildPacket_S2CEnter(playerId, enterPacket))
    {
        return false;
    }

    if (!session.Send(std::move(enterPacket)))
    {
        return false;
    }

    std::cout << "C2S_Enter received\n";
    return true;
}

bool Server::HandleInput(Session& session, std::span<const Z1::Protocol::Byte> payload)
{
    // 입장하지 않은 플레이어
    if (!session.IsEntered())
    {
        return false;
    }

    InputCommand input;
    if (!ParsePayload_C2SInput(payload, input))
    {
        return false;
    }

    return _overworld.SetInput(*session.GetPlayerId(), input);
}

void Server::AcceptLoop()
{
    while (!_stopRequested)
    {
        Endpoint clientaddr;
        Socket client = _listener.Accept(&clientaddr);

        if (!client.IsValid())
        {
            if (_stopRequested)
            {
                break;
            }

            continue;
        }

        // Accept 스레드는 수신 소켓 큐에 push만 하자.
        {
            std::lock_guard lock(_acceptMutex);
            _acceptedSockets.push_back(std::move(client));
        }
    }
}

/// <summary>
/// IOCP Worker 스레드가 네트워크 이벤트 처리하는 함수
/// </summary>
void Server::IOLoop()
{
    using namespace std::chrono;
    using Clock = steady_clock;

    constexpr auto TickInterval = milliseconds(50);    // tick 주기는 50ms(== 초당 20번: 20hz)
    constexpr int MaxCatchupTicks = 5;  // 한 Tick Loop 안에서 과거 Tick을 최대 몇 번 보정할 지
    auto nextTick = Clock::now() + TickInterval;    // 다음 Tick을 호출해야하는 시각

    while (true)
    {
        ProcessAcceptedSockets();

        const auto now = Clock::now();
        int tickCount = 0;  // 이번 while 안에서 처리한 catch-up 틱 횟수
        while (now >= nextTick && tickCount < MaxCatchupTicks)
        {
            // 일반적인 상황에선 1회만 수행되나, breakpoint 등으로 서버가 잠깐 멈추면 
            // now가 이전에 설정한 nextTick보다 뒷 시간이 됨
            // 그러면 그 간격만큼 Tick을 몰아서 실행하게 되는데
            // 그 횟수의 상한이 MaxCatchupTick
            Tick();
            nextTick += TickInterval;
            ++tickCount;
        }

        // 횟수 상한을 다 채웠는데도 이 상태라면(즉 너무 오래 멈췄다면) 강제로 시간 보정
        if (now >= nextTick)
        {
            nextTick = now + TickInterval;
        }

        const auto remaining = nextTick - Clock::now();
        const long long remainingMs = duration_cast<milliseconds>(remaining).count();
        const DWORD timeoutMs = (DWORD)((remainingMs > 0 ? remainingMs : 1));

        CompletionEvent event = _cp.Dequeue(timeoutMs);
        if (event.IsTimeout())
        {
            // 무한 대기(INFINITE)가 아닌 Tick 간격만큼만 기다리도록 변경했으므로,
            // 다음 While-loop 진입에서 Tick 실행 여부를 다시 판단한다.
            continue;
        }

        if (event.IsShutdown())
        {
            // 종료를 위해 Post~에서 completionKey에 nullptr을 넣었음
            break;
        }

        // completionKey를 Session*로 넘겼었음!
        Session* session = (Session*)event.completionKey;
        if (!event.succeeded)
        {
            // 각 세션은 accept 시 unique_ptr로 서버에서 관리하고,
            // 아직 vector에서 세션을 제거하는 로직이 없어 유효함
            if (session != nullptr)
            {
                CloseSession(*session);  
            }

            continue;
        }

        // case 1: Recv 이벤트 완료
        if (event.overlapped == session->GetRecvOverlapped())
        {
            // 상대방이 전송한 데이터가 0이면 접속 종료를 요청한 것
            if (event.bytesTransferred == 0)
            {
                std::cout << "Client Disconnected\n";
                CloseSession(*session);
                continue;
            }

            if (!session->HandleRecv(event.bytesTransferred))
            {
                CloseSession(*session);
                continue;
            }

            bool isValidSession = true;
            Packet packet;
            while (session->TryPopRecvdPacket(packet))
            {
                if (!HandleClientPacket(*session, packet))
                {
                    CloseSession(*session);
                    isValidSession = false;
                    break;
                }
            }

            if (!isValidSession)
            {
                continue;
            }

            if (!_stopRequested && !session->PostRecv())
            {
                CloseSession(*session);
            }

            continue;
        }

        // case 2: Send 이벤트 완료
        if (event.overlapped == session->GetSendOverlapped())
        {
            // Send Completion에서 전송한 바이트 수가 0이라면 비정상 전송
            if (!session->HandleSend(event.bytesTransferred))
            {
                CloseSession(*session);
            }

            continue;
        }

        // 등록하지 않은 OVERLAPPED 완료 통지
        CloseSession(*session);
    }
}

void Server::ProcessAcceptedSockets()
{
    // 수신 스레드에서 큐잉한 소켓들을 지역 변수 큐로 바꾼 뒤 Register 처리
    // 왜 지역 변수로 옮기는가?
    std::deque<Socket> acceptedSockets;
    {
        std::lock_guard lock(_acceptMutex);
        acceptedSockets.swap(_acceptedSockets);
    }

    for (Socket& socket : acceptedSockets)
    {
        RegisterAcceptedSocket(std::move(socket));
    }
}

/*
* 접속한 클라를 Session화 시켜서 컨테이너에 보관
* IOLoop 스레드가 이를 매 프레임 처음에 처리함
*/
bool Server::RegisterAcceptedSocket(Net::Socket&& clientSocket)
{
    auto clientSession = std::make_unique<Session>(std::move(clientSocket));
    Session* clientSessionPtr = clientSession.get();

    /*
    * 연결된 클라를 IOCP 큐가 관찰하도록 Register.
    * 이때 CompletionKey를 Session*로 설정하여 IOCP Worker에서 해당 세션을 찾을 수 있게 함
    * 이 Session 객체는 Heap에 있기 때문에, 해당 세션의 주소(Session*)가 바뀌지 않음
    * 단, IOCP Completion이 있는 동안 Session을 Vector에서 erase 시키면 안됨
    * ULONG_PTR을 중간에 거쳐야 하는가?
    */

    if (!_cp.Associate(clientSessionPtr->GetNativeHandle(), (ULONG_PTR)clientSessionPtr))
    {
        // 등록 실패
        clientSessionPtr->Close();
        return false;
    }

    if (!clientSessionPtr->PostRecv())
    {
        // WSARecv 실패
        clientSessionPtr->Close();
        return false;
    }

    _sessions.push_back(std::move(clientSession));
    std::cout << "Client Connected!\n";
    return true;
}

void Server::CloseSession(Session& session)
{
    if (const auto playerId = session.GetPlayerId())
    {
        _overworld.RemovePlayer(*playerId);
    }

    session.Close();
}

// 서버 시뮬레이션 역할을 하는 Tick
// 플레이어, 적, 투사체 상태 갱신
void Server::Tick()
{
    //std::cout << "Server::Tick(10ms)\n";
    _overworld.Tick();
    BroadcastCombatEvents(_overworld.TakeCombatEvents());
    BroadcastWorldSnapshot();
}

/// <summary>
/// Simulation에서 공격 요청마다 쌓은 PendingCombatEvents를
/// 매 틱에서 꺼내서 순회해
/// 패킷을 하나 만들고, 같은 방에 속한 모든 세션에게 전파
/// </summary>
/// <param name="events"></param>
void Server::BroadcastCombatEvents(const std::vector<PendingCombatEvent>& events)
{
    for (const PendingCombatEvent& pending : events)
    {
        std::vector<Byte> packet;
        if (!BuildPacket_S2CCombatEvent(pending.event, packet))
        {
            continue;
        }

        for (const auto& session : _sessions)
        {
            if (!session->IsEntered())
            {
                continue;
            }

            auto playerId = session->GetPlayerId();
            if (!playerId || !_overworld.IsPlayerInRoom(*playerId, pending.room))
            {
                continue;
            }

            // 패킷 하나를 돌려쓰는 입장이라, move()가 아니라 임시값 만들어서 넘겨야 함
            if(!session->Send(std::vector<Byte>(packet)))
            {
                CloseSession(*session);
            }
        }
    }
}

/// <summary>
/// 1. BuildPlayerSnapshot() 호출
/// 2. payload 직렬화
/// 3. BuildPacket(S2C_WorldSnapshot, ...)
/// 4. 현재 입장한 Session들의 송신 큐에 추가
/// </summary>
void Server::BroadcastWorldSnapshot()
{
    for (const std::unique_ptr<Session>& session : _sessions)
    {
        if (!session->IsEntered())
        {
            continue;
        }

        // playerID 기반 Snapshot을 각각 만들어 송신
        auto playerId = session->GetPlayerId();
        WorldSnapshot snapshot = _overworld.BuildSnapshot(*playerId);

        std::vector<Byte> packet;
        if (!BuildPacket_S2CWorldSnapshot(snapshot, packet) || !session->Send(std::move(packet)))
        {
            CloseSession(*session);
        }
    }
}
