#include "pipe_comm.hpp"
#include <iostream>

PipeServer::PipeServer(const std::string& pipeName) : name("\\\\.\\pipe\\" + pipeName), hPipe(INVALID_HANDLE_VALUE), running(false) {}

void PipeServer::Start(std::function<void(const std::string&)> onMessage) {
    running = true;
    std::thread([this, onMessage]() {
        while (running) {
            hPipe = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024 * 16, 1024 * 16, 0, nullptr);
            if (hPipe == INVALID_HANDLE_VALUE) continue;

            if (ConnectNamedPipe(hPipe, nullptr)) {
                char buffer[1024 * 16];
                DWORD bytesRead;
                while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
                    buffer[bytesRead] = '\0';
                    onMessage(std::string(buffer));
                }
            }
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        }
    }).detach();
}

void PipeServer::Send(const std::string& message) {
    if (hPipe != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hPipe, message.c_str(), message.length(), &bytesWritten, nullptr);
    }
}
