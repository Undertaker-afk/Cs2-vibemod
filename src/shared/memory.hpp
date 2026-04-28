#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>

class Memory {
public:
    static uintptr_t PatternScan(const char* moduleName, const char* pattern) {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) return 0;

        MODULEINFO moduleInfo;
        GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo));

        uintptr_t start = (uintptr_t)moduleInfo.lpBaseOfDll;
        uintptr_t end = start + moduleInfo.SizeOfImage;

        std::vector<int> bytes = PatternToBytes(pattern);
        size_t size = bytes.size();
        int* data = bytes.data();

        for (uintptr_t i = start; i < end - size; ++i) {
            bool found = true;
            for (size_t j = 0; j < size; ++j) {
                if (data[j] != -1 && data[j] != *(BYTE*)(i + j)) {
                    found = false;
                    break;
                }
            }
            if (found) return i;
        }
        return 0;
    }

private:
    static std::vector<int> PatternToBytes(const char* pattern) {
        std::vector<int> bytes;
        char* start = const_cast<char*>(pattern);
        char* end = const_cast<char*>(pattern) + strlen(pattern);

        for (char* current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?') ++current;
                bytes.push_back(-1);
            } else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
    }
};
