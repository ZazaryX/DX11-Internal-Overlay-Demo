#include "includes.h"
#include "MemoryDefs.h"
        std::pair<std::vector<BYTE>, std::string> ParsePattern(const std::string& pattern) {
            std::vector<BYTE> bytes;
            std::string mask;
            std::istringstream iss(pattern);
            std::string byteStr;

            while (iss >> byteStr) {
                if (byteStr == "?" || "??") {
                    bytes.push_back(0);
                    mask += '?';
                }
                else {
                    bytes.push_back(static_cast<BYTE>(std::stoi(byteStr, nullptr, 16)));
                    mask += 'x';
                }
            }
            return { bytes, mask };
        }

        bool CompareBytes(const BYTE* data, const std::vector<BYTE>& pattern, const std::string& mask) {
            for (size_t i = 0; i < pattern.size(); ++i) {
                if (mask[i] == 'x' && data[i] != pattern[i]) {
                    return false;
                }
            }
            return true;
        }

        uintptr_t FindPattern(HANDLE hProcess, const std::vector<BYTE>& pattern, const std::string& mask) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            uintptr_t start = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
            uintptr_t end = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
            MEMORY_BASIC_INFORMATION mbi;

            std::vector<BYTE> buffer;
            SIZE_T bytesRead;

            for (uintptr_t addr = start; addr < end; addr += mbi.RegionSize) {
                if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) {
                    if ((mbi.State == MEM_COMMIT) && (mbi.Protect & PAGE_EXECUTE_READWRITE || mbi.Protect & PAGE_EXECUTE_READ || mbi.Protect & PAGE_READWRITE)) {
                        buffer.resize(mbi.RegionSize);
                        if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                            for (size_t i = 0; i < bytesRead - pattern.size(); ++i) {
                                if (CompareBytes(buffer.data() + i, pattern, mask)) {
                                    return addr + i;
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }
