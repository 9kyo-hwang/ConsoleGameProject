#include "pch.h"
#include "Server.h"
#include <Sockets/Endpoint.h>
#include <iostream>

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

    if (offset != payload.size())
    {
        return false;
    }

    if (version != ProtocolVersion)
    {
        return false;
    }

    std::cout << "C2S_Enter received\n";
    return true;
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
    while (true)
    {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED* overlapped = nullptr;

        const BOOL completed = ::GetQueuedCompletionStatus(_completionPort, &bytesTransferred, &completionKey, &overlapped, INFINITE);
        
        // 종료를 위해 Post~에서 completionKey에 nullptr을 넣었음
        if (completionKey == 0 && overlapped == nullptr)
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
        if (!completed)
        {
            session->Close();
            continue;
        }

        // 상대방이 전송한 데이터가 0이면 접속 종료를 요청한 것
        if (bytesTransferred == 0)
        {
            std::cout << "Client Disconnected\n";
            session->Close();
            continue;
        }

        // 이건 무슨 검사지?
        if (overlapped != session->GetRecvOverlapped())
        {
            session->Close();
            continue;
        }

        if (!session->HandleRecv(bytesTransferred))
        {
            session->Close();
            continue;
        }

        bool isValidSession = true;
        Session::RecvdPacket packet;
        while (session->TryPopRecvdPacket(packet))
        {
            if (!HandleClientPacket(*session, packet))
            {
                session->Close();
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
            session->Close();
        }
    }
}
