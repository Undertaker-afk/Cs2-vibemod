#include "injector.hpp"
#include <fstream>
#include <iostream>

// Minimal position-independent shellcode for loading a DLL from memory
// In a real scenario, this would be compiled from ASM
const BYTE Injector::ShellcodeBlob[] = {
    0x48, 0x89, 0x4c, 0x24, 0x08, // mov [rsp+8], rcx (placeholder)
    // ... actual bytes would go here ...
    0xC3 // ret
};
const DWORD Injector::ShellcodeSize = sizeof(Injector::ShellcodeBlob);

bool Injector::Inject(DWORD processId, const std::string& dllPath) {
    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (file.fail()) {
        std::cerr << "Failed to open DLL: " << dllPath << std::endl;
        return false;
    }

    auto fileSize = file.tellg();
    char* buffer = new char[fileSize];
    file.seekg(0, std::ios::beg);
    file.read(buffer, fileSize);
    file.close();

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProc) {
        std::cerr << "Failed to open process: " << GetLastError() << std::endl;
        delete[] buffer;
        return false;
    }

    bool success = ManualMap(hProc, buffer);

    CloseHandle(hProc);
    delete[] buffer;
    return success;
}

bool Injector::ManualMap(HANDLE hProc, const char* buffer) {
    IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)buffer;
    IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)(buffer + pDosHeader->e_lfanew);

    void* pTargetBase = VirtualAllocEx(hProc, nullptr, pNtHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pTargetBase) return false;

    WriteProcessMemory(hProc, pTargetBase, buffer, pNtHeaders->OptionalHeader.SizeOfHeaders, nullptr);

    IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
    for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; ++i) {
        WriteProcessMemory(hProc, (void*)((BYTE*)pTargetBase + pSectionHeader[i].VirtualAddress), buffer + pSectionHeader[i].PointerToRawData, pSectionHeader[i].SizeOfRawData, nullptr);
    }

    ManualMapData data;
    data.pLoadLibraryA = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    data.pGetProcAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");
    data.pModuleBase = pTargetBase;

    void* pDataRemote = VirtualAllocEx(hProc, nullptr, sizeof(ManualMapData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(hProc, pDataRemote, &data, sizeof(ManualMapData), nullptr);

    void* pShellcodeRemote = VirtualAllocEx(hProc, nullptr, ShellcodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(hProc, pShellcodeRemote, ShellcodeBlob, ShellcodeSize, nullptr);

    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)pShellcodeRemote, pDataRemote, 0, nullptr);
    if (!hThread) return false;

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    return true;
}
