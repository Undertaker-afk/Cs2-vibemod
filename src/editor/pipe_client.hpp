#pragma once
#include <windows.h>
#include <string>

class PipeClient {
public:
    PipeClient(const std::string& pipeName);
    bool Send(const std::string& message);

private:
    std::string name;
};
