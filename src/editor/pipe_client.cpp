#include "pipe_client.hpp"

PipeClient::PipeClient(const std::string& pipeName) : name("\\\\.\\pipe\\" + pipeName) {}

bool PipeClient::Send(const std::string& message) {
    HANDLE hPipe = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPipe == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten;
    WriteFile(hPipe, message.c_str(), message.length(), &bytesWritten, nullptr);
    CloseHandle(hPipe);
    return true;
}
