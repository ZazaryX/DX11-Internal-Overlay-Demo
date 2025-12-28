#include "includes.h"
#include "MemoryDefs.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present)(IDXGISwapChain*, UINT, UINT);
Present oPresent = nullptr;

HWND window = nullptr;
WNDPROC oWndProc = nullptr;

ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;

bool init = false;
bool ShowMenu = true;

std::atomic<bool> updateThreadRunning{ false };

struct GameData {
    int currentHP = 0;
    int newHP = 100;
    bool updateHP = false;

    int currentMoney = 0;
    int newMoney = 0;
    bool updateMoney = false;

    int currentKills = 0;
    int newKills = 0;
    bool updateKills = false;

    int currentSouls = 0;
    int newSouls = 0;
    bool updateSouls = false;

    double currentTime = 0.0;
    double newTime = 0.0;
    bool updateTime = false;

    uintptr_t foundAddressCD = 0;
    std::vector<BYTE> nops = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    std::vector<BYTE> originalBytesCD;
    bool updateCD = false;

    std::string status = "Initializing...";
};

GameData gameData;

void UpdateValuesThread() {
    while (updateThreadRunning) {
        if (!mem.hProcess || mem.hProcess == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        uintptr_t base = mem.gameModule + BaseAddr;

        uintptr_t HpBase = findDMAAddy(mem.hProcess, base, HPOffsets);
        uintptr_t MoneyBase = findDMAAddy(mem.hProcess, base, MoneyOffsets);
        uintptr_t KillsBase = findDMAAddy(mem.hProcess, base, KillsOffsets);
        uintptr_t SoulsBase = findDMAAddy(mem.hProcess, base, RCOffsets);
        uintptr_t TimeBase = findDMAAddy(mem.hProcess, base, TimeOffsets);

        int tempInt;
        if (HpBase && ReadProcessMemory(mem.hProcess, (LPCVOID)HpBase, &tempInt, sizeof(int), nullptr))
            gameData.currentHP = tempInt;

        if (MoneyBase && ReadProcessMemory(mem.hProcess, (LPCVOID)MoneyBase, &tempInt, sizeof(int), nullptr))
            gameData.currentMoney = tempInt;

        if (KillsBase && ReadProcessMemory(mem.hProcess, (LPCVOID)KillsBase, &tempInt, sizeof(int), nullptr))
            gameData.currentKills = tempInt;

        if (SoulsBase && ReadProcessMemory(mem.hProcess, (LPCVOID)SoulsBase, &tempInt, sizeof(int), nullptr))
            gameData.currentSouls = tempInt;

        double tempDouble;
        if (TimeBase && ReadProcessMemory(mem.hProcess, (LPCVOID)TimeBase, &tempDouble, sizeof(double), nullptr))
            gameData.currentTime = tempDouble;

        if (gameData.updateHP && HpBase) {
            WriteProcessMemory(mem.hProcess, (LPVOID)HpBase, &gameData.newHP, sizeof(int), nullptr);
            gameData.currentHP = gameData.newHP;
            gameData.updateHP = false;
            gameData.status = "HP updated to " + std::to_string(gameData.newHP);
        }

        if (gameData.updateMoney && MoneyBase) {
            WriteProcessMemory(mem.hProcess, (LPVOID)MoneyBase, &gameData.newMoney, sizeof(int), nullptr);
            gameData.currentMoney = gameData.newMoney;
            gameData.updateMoney = false;
            gameData.status = "Money updated to " + std::to_string(gameData.newMoney);
        }

        if (gameData.updateKills && KillsBase) {
            WriteProcessMemory(mem.hProcess, (LPVOID)KillsBase, &gameData.newKills, sizeof(int), nullptr);
            gameData.currentKills = gameData.newKills;
            gameData.updateKills = false;
        }

        if (gameData.updateSouls && SoulsBase) {
            WriteProcessMemory(mem.hProcess, (LPVOID)SoulsBase, &gameData.newSouls, sizeof(int), nullptr);
            gameData.currentSouls = gameData.newSouls;
            gameData.updateSouls = false;
        }

        if (gameData.updateTime && TimeBase) {
            WriteProcessMemory(mem.hProcess, (LPVOID)TimeBase, &gameData.newTime, sizeof(double), nullptr);
            gameData.currentTime = gameData.newTime;
            gameData.updateTime = false;
        }

        if (gameData.foundAddressCD && gameData.originalBytesCD.size() == gameData.nops.size()) {
            if (gameData.updateCD) {
                WriteProcessMemory(mem.hProcess, (LPVOID)gameData.foundAddressCD, gameData.nops.data(), gameData.nops.size(), nullptr);
            }
            else {
                WriteProcessMemory(mem.hProcess, (LPVOID)gameData.foundAddressCD, gameData.originalBytesCD.data(), gameData.originalBytesCD.size(), nullptr);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void InitImGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == VK_HOME) {
        ShowMenu = !ShowMenu;
        return true;
    }

    if (ShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            pDevice->GetImmediateContext(&pContext);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;

            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();

            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

            GetWindowThreadProcessId(window, &mem.pid);

            mem.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, mem.pid);
            if (!mem.hProcess) {
                gameData.status = "Failed to open process";
                return oPresent(pSwapChain, SyncInterval, Flags);
            }

            mem.gameModule = GetModuleBaseAddress(L"libhl.dll");
            if (!mem.gameModule) {
                gameData.status = "libhl.dll not found";
                return oPresent(pSwapChain, SyncInterval, Flags);
            }

            auto [pattern, mask] = ParsePattern(patternCD);
            gameData.foundAddressCD = FindPattern(mem.hProcess, pattern, mask);

            if (gameData.foundAddressCD) {
                gameData.originalBytesCD.resize(gameData.nops.size());
                ReadProcessMemory(mem.hProcess, (LPCVOID)gameData.foundAddressCD, gameData.originalBytesCD.data(), gameData.originalBytesCD.size(), nullptr);
                gameData.status = "Cooldown address found: 0x" + std::to_string(gameData.foundAddressCD);
            }
            else {
                gameData.status = "Cooldown pattern not found";
            }

            InitImGui();
            init = true;

            updateThreadRunning = true;
            std::thread(UpdateValuesThread).detach();

            AllocConsole();
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            printf("Dead Cells cheat injected successfully!\n");
            printf("Cooldown addr: 0x%llX\n", gameData.foundAddressCD);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ShowMenu) {
        ImGui::Begin("[Dead Cells] Zazary Realm", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Main")) {
                ImGui::Text("Status: %s", gameData.status.c_str());

                ImGui::Text("Current HP: %d", gameData.currentHP);
                ImGui::InputInt("New HP", &gameData.newHP);
                if (ImGui::Button("Set HP")) gameData.updateHP = true;

                ImGui::Separator();

                ImGui::Text("Current Money: %d", gameData.currentMoney);
                ImGui::InputInt("New Money", &gameData.newMoney);
                if (ImGui::Button("Set Money")) gameData.updateMoney = true;

                ImGui::Separator();

                ImGui::Text("Current Kills: %d", gameData.currentKills);
                ImGui::InputInt("New Kills", &gameData.newKills);
                if (ImGui::Button("Set Kills")) gameData.updateKills = true;

                ImGui::Separator();

                ImGui::Text("Current Souls: %d", gameData.currentSouls);
                ImGui::InputInt("New Souls", &gameData.newSouls);
                if (ImGui::Button("Set Souls")) gameData.updateSouls = true;

                ImGui::Separator();

                ImGui::Text("Current Time: %.2f", gameData.currentTime);
                ImGui::InputDouble("New Time", &gameData.newTime, 0.1, 1.0, "%.2f");
                if (ImGui::Button("Set Time")) gameData.updateTime = true;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Toggle's")) {
                ImGui::Checkbox("No Cooldown", &gameData.updateCD);
                ImGui::EndTabItem();
            }


            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved) {
    bool hooked = false;
    while (!hooked) {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            hooked = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        updateThreadRunning = false;

        kiero::shutdown();

        if (init) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        if (mainRenderTargetView) mainRenderTargetView->Release();
        if (pContext) pContext->Release();
        if (pDevice) pDevice->Release();
        if (mem.hProcess && mem.hProcess != INVALID_HANDLE_VALUE) CloseHandle(mem.hProcess);

        break;
    }
    return TRUE;
}