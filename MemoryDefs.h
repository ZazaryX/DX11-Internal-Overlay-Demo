#pragma once
#include "includes.h"

struct Memory {
    DWORD pid;
    HANDLE hProcess;
    uintptr_t baseAddress;
    size_t moduleSize;
    uintptr_t gameModule;
};
extern Memory mem;

struct Bases {
    uintptr_t MoneyBase;
    uintptr_t KillsBase;
    uintptr_t RCBase;
    uintptr_t TimeBase;
};
extern Bases bases;

extern uintptr_t BaseAddr;
extern std::string patternCD;
extern std::string patternHP;
extern std::vector<unsigned int> MoneyOffsets;
extern std::vector<unsigned int> RCOffsets;
extern std::vector<unsigned int> TimeOffsets;
extern std::vector<unsigned int> KillsOffsets;
extern std::vector<unsigned int> HPOffsets;

std::pair<std::vector<BYTE>, std::string> ParsePattern(const std::string& pattern);
uintptr_t FindPattern(HANDLE hProcess, const std::vector<BYTE>& pattern, const std::string& mask);
bool GetMainModuleInfo(DWORD pid, uintptr_t& baseAddress, size_t& moduleSize);
uintptr_t GetModuleBaseAddress(const wchar_t* modName);
uintptr_t findDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets);
