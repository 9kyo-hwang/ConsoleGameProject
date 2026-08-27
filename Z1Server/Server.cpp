#include "pch.h"
#include "Server.h"
#include <Sockets/Endpoint.h>
#include <chrono>

using namespace Net;

bool Server::Start(std::uint16_t port)
{
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

    _completionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!_completionPort)
    {
        return false;
    }

    _ioThread = std::thread(&Server::IOLoop, this);
    _acceptThread = std::thread(&Server::AcceptLoop, this);
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
    ::PostQueuedCompletionStatus(_completionPort, 0, 0, nullptr);

    if (_ioThread.joinable())
    {
        _ioThread.join();
    }
}

// 수신 패킷의 타입을 보고 타입 별 핸들 함수를 호출하는 역할
bool Server::HandleClientPacket(Session& session, const Session::RecvdPacket& packet)
{
    std::span<const Byte> payload(packet.payload.data(), packet.payload.size());
    switch ((PacketType)packet.rawType)
    {
    case PacketType::C2S_Enter: return HandleEnter(session, payload);
    case PacketType::C2S_Input: return HandleInput(session, payload);
    default: return false;
    }
}

// C2S_Enter 패킷 처리 함수
bool Server::HandleEnter(Session& session, std::span<const Z1::Protocol::Byte> payload)
{
    // 1. payload 크기가 sizeof(uint16_t)인지
    // 2. protocol version이 ProtocolVersion인지

    if (payload.size() != sizeof(std::uint16_t))
    {
        return false;
    }

    std::size_t offset = 0;
    std::uint16_t version = 0;

    if (!ReadU16(payload, offset, version))
    {
        return false;
    }

    // 읽은 크기가 페이로드 크기와 다르거나, 버전이 안맞으면 false
    if (offset != payload.size() || version != ProtocolVersion)
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
    std::vector<Byte> enterPayload;
    WriteU16(enterPayload, ProtocolVersion);
    WriteU32(enterPayload, *session.GetPlayerId()); // 패킷 페이로드가 됨

    std::vector<Byte> enterPacket;
    if (!BuildPacket(PacketType::S2C_Enter, enterPayload, enterPacket))
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

    // uint32(시퀀스) + uint8(이동 방향) + uint8(공격 유무)
    if (payload.size() != sizeof(std::uint32_t) + 2)
    {
        return false;
    }

    std::size_t offset = 0;
    std::uint32_t sequence = 0;
    std::uint8_t direction = 0, actionFlags = 0;

    if (!ReadU32(payload, offset, sequence) || !ReadU8(payload, offset, direction) || !ReadU8(payload, offset, actionFlags))
    {
        return false;
    }

    // 4 + 1 + 1 읽었는데 offset이 payload 크기랑 다르면 문제
    if (offset != payload.size())
    {
        return false;
    }

    // 0 ~ 4 의 값이 아니라면 문제
    if (direction > (std::uint8_t)(MoveDirection::Right))
    {
        return false;
    }

    // [11111...0]이랑 & 했는데 0이 아니라면 flag로 2 이상의 값이 들어왔다는 뜻
    if ((actionFlags & ~ValidInputActions) != 0)
    {
        return false;
    }

    const InputCommand input{ sequence, (MoveDirection)direction, actionFlags };
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

        /*
        * 접속한 클라를 Session화 시켜서 컨테이너에 보관
        * IOCP associate
        * WSARecv 등록
        */

        // 대충 이런 형태가 되려나?
        auto session = std::make_unique<Session>(std::move(client));    // Socket도 복사가 막혀있음
        Session* sessionPtr = session.get();
        _sessions.push_back(std::move(session));

        // 
        /*
        * 연결된 클라를 IOCP 큐가 관찰하도록 Register. 
        * 이때 CompletionKey를 Session*로 설정하여 IOCP Worker에서 해당 세션을 찾을 수 있게 함
        * 이 Session 객체는 Heap에 있기 때문에, 해당 세션의 주소(Session*)가 바뀌지 않음
        * 단, IOCP Completion이 있는 동안 Session을 Vector에서 erase 시키면 안됨
        * ULONG_PTR을 중간에 거쳐야 하는가?
        */
        HANDLE clientHandle = (HANDLE)sessionPtr->GetNativeHandle();
        HANDLE associatedPort = ::CreateIoCompletionPort(clientHandle, _completionPort, (ULONG_PTR)sessionPtr, 0);

        // 등록 실패
        if (associatedPort == nullptr)
        {
            sessionPtr->Close();
            continue;
        }

        // WSARecv 실패
        if (!sessionPtr->PostRecv())
        {
            sessionPtr->Close();
            continue;
        }

        std::cout << "Client Connected!\n";
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

        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED* overlapped = nullptr;

        // 대기시간을 INFINITE로 넘기면 네트워크 이벤트가 올 때까지 영원히 sleep
        // 따라서 다음 tick까지 남은 시간으로 변경
        const BOOL dequeueSuccess = ::GetQueuedCompletionStatus(_completionPort, &bytesTransferred, &completionKey, &overlapped, timeoutMs);
        const DWORD completionError = dequeueSuccess ? ERROR_SUCCESS : ::WSAGetLastError();
        
        // 실패 원인이 TIMEOUT이라면?
        if (!dequeueSuccess && completionError == WAIT_TIMEOUT)
        {
            // 무한 대기(INFINITE)가 아닌 Tick 간격만큼만 기다리도록 변경했으므로.
            // 다음 While-loop 진입에서 Tick 실행 여부를 다시 판단한다.
            continue;
        }

        // 종료를 위해 Post~에서 completionKey에 nullptr을 넣었음
        if (dequeueSuccess && completionKey == 0 && overlapped == nullptr)
        {
            break;
        }

        // 예상치 못한 completion 방어
        if (completionKey == 0 || overlapped == nullptr)
        {
            break;
        }

        // completionKey를 Session*로 넘겼었음!
        // 각 세션은 accept 시 unique_ptr로 서버에서 관리하고,
        // 아직 vector에서 세션을 제거하는 로직이 없어 유효함
        Session* session = (Session*)completionKey;

        if (!dequeueSuccess)
        {
            CloseSession(*session);
            continue;
        }

        // case 1: Recv 이벤트 완료
        if (overlapped == session->GetRecvOverlapped())
        {
            // 상대방이 전송한 데이터가 0이면 접속 종료를 요청한 것
            if (bytesTransferred == 0)
            {
                std::cout << "Client Disconnected\n";
                CloseSession(*session);
                continue;
            }

            if (!session->HandleRecv(bytesTransferred))
            {
                CloseSession(*session);
                continue;
            }

            bool isValidSession = true;
            Session::RecvdPacket packet;
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

            // 일단 수신 확인만 하자. 이후 WSASend로 echo
            if (!_stopRequested && !session->PostRecv())
            {
                CloseSession(*session);
            }

            continue;
        }

        // case 2: Send 이벤트 완료
        if (overlapped == session->GetSendOverlapped())
        {
            // Send Completion에서 전송한 바이트 수가 0이라면 비정상 전송
            if (!session->HandleSend(bytesTransferred))
            {
                CloseSession(*session);
            }

            continue;
        }

        // 등록하지 않은 OVERLAPPED 완료 통지
        CloseSession(*session);
    }
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
}
