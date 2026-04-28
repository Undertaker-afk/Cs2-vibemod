#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <functional>

class PipeServer {
public:
    PipeServer(const std::string& pipeName);
    void Start(std::function<void(const std::string&)> onMessage);
    void Send(const std::string& message);

private:
    std::string name;
    HANDLE hPipe;
    bool running;
};
