#include <windows.h>
#include <iostream>
#include "lua_engine.hpp"
#include "pipe_comm.hpp"
#include "memory.hpp"

LuaEngine* g_Lua = nullptr;
PipeServer* g_Pipe = nullptr;

void HandleEditorMessage(const std::string& msg) {
    // Message format could be "EXEC:code" or "PING"
    if (msg.substr(0, 5) == "EXEC:") {
        std::string code = msg.substr(5);
        if (g_Lua) g_Lua->ExecuteString(code);
    }
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "[VibeCoder] DLL Initializing..." << std::endl;

    g_Lua = new LuaEngine();
    g_Pipe = new PipeServer("VibeCoderPipe");

    g_Pipe->Start(HandleEditorMessage);

    std::cout << "[VibeCoder] Ready. Waiting for Editor..." << std::endl;

    while (true) {
        // Main loop for hooks/rendering if needed
        Sleep(100);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}
