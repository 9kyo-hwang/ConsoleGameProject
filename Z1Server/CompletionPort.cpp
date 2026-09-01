#include "pch.h"
#include "CompletionPort.h"

CompletionPort::~CompletionPort()
{
    Close();
}

bool CompletionPort::Create()
{
    if (_handle != INVALID_HANDLE_VALUE) return false;

    _handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    return _handle != nullptr;
}

bool CompletionPort::Associate(SOCKET socket, ULONG_PTR completionKey)
{
    const HANDLE result = ::CreateIoCompletionPort((HANDLE)socket, _handle, completionKey, 0);
    return result == _handle;
}

void CompletionPort::Close() noexcept
{
    if (_handle)
    {
        CloseHandle(_handle);
        _handle = nullptr;
    }
}

bool CompletionPort::IsValid() const noexcept
{
    return _handle && _handle != INVALID_HANDLE_VALUE;
}

CompletionEvent CompletionPort::Dequeue(DWORD timeoutMs) const
{
    CompletionEvent event;
    event.succeeded = ::GetQueuedCompletionStatus(
        _handle, 
        &event.bytesTransferred, 
        &event.completionKey, 
        &event.overlapped, 
        timeoutMs
    );

    event.error = event.succeeded ? ERROR_SUCCESS : ::GetLastError();
    return event;
}

bool CompletionPort::PostShutdown() const
{
    return _handle && ::PostQueuedCompletionStatus(_handle, 0, 0, nullptr) != FALSE;
}
