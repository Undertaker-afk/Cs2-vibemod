#pragma once
#include <windows.h>
#include <string>

struct ManualMapData {
    void* pLoadLibraryA;
    void* pGetProcAddress;
    void* pModuleBase;
};

// Shellcode must be written carefully to be position-independent
// For this task, we assume the injector builds a relocatable shellcode blob
class Injector {
public:
    static bool Inject(DWORD processId, const std::string& dllPath);
private:
    static bool ManualMap(HANDLE hProc, const char* buffer);
    // Use a static byte array instead of a function pointer for the shellcode
    static const BYTE ShellcodeBlob[];
    static const DWORD ShellcodeSize;
};
