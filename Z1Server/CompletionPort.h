#pragma once

#include <WinSock2.h>

struct CompletionEvent
{
    bool succeeded = false;
    DWORD bytesTransferred = 0;
    ULONG_PTR completionKey = 0;
    OVERLAPPED* overlapped = nullptr;
    DWORD error = ERROR_SUCCESS;

    bool IsTimeout() const noexcept { return !succeeded && error == WAIT_TIMEOUT; }
    bool IsShutdown() const noexcept { return succeeded && completionKey == 0 && overlapped == nullptr; }
};

class CompletionPort
{
public:
    CompletionPort() = default;
    ~CompletionPort();

    CompletionPort(const CompletionPort&) = delete;
    CompletionPort& operator=(const CompletionPort&) = delete;

    bool Create();
    bool Associate(SOCKET socket, ULONG_PTR completionKey);
    CompletionEvent Dequeue(DWORD timeoutMs) const;
    bool PostShutdown() const;

    void Close() noexcept;
    bool IsValid() const noexcept;

private:
    HANDLE _handle = INVALID_HANDLE_VALUE;
};

