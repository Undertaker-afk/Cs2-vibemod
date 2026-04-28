#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include "injector.hpp"

DWORD GetProcessIdByName(const std::string& processName) {
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32First(hSnapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
    }
    return processId;
}

int main(int argc, char* argv[]) {
    std::cout << "--- VibeCoder Injector ---" << std::endl;

    std::string processName = "cs2.exe";
    std::string dllPath = "vibecoder.dll";

    if (argc > 1) processName = argv[1];
    if (argc > 2) dllPath = argv[2];

    DWORD pid = GetProcessIdByName(processName);
    if (pid == 0) {
        std::cerr << "Could not find " << processName << ". Make sure the game is running." << std::endl;
        return 1;
    }

    std::cout << "Found " << processName << " (PID: " << pid << ")" << std::endl;
    std::cout << "Injecting " << dllPath << "..." << std::endl;

    if (Injector::Inject(pid, dllPath)) {
        std::cout << "Injection successful!" << std::endl;
    } else {
        std::cerr << "Injection failed." << std::endl;
        return 1;
    }

    return 0;
}
