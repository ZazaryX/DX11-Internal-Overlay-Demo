#include "includes.h"
#include "MemoryDefs.h"
    uintptr_t GetModuleBaseAddress(const wchar_t* modName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, mem.pid);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W moduleEntry;
            moduleEntry.dwSize = sizeof(MODULEENTRY32W);
            if (Module32FirstW(hSnapshot, &moduleEntry)) {
                do {
                    if (wcscmp(modName, moduleEntry.szModule) == 0) {
                        CloseHandle(hSnapshot);
                        return (uintptr_t)moduleEntry.modBaseAddr;
                    }
                } while (Module32NextW(hSnapshot, &moduleEntry));
            }
            CloseHandle(hSnapshot);
        }
        return 0;
    }

    uintptr_t findDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets) {
        uintptr_t addr = ptr;
        for (unsigned int i = 0; i < offsets.size(); ++i) {
            if (!ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), 0)) {
                return 0;
            }
            addr += offsets[i];
        }
        return addr;
    }
