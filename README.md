# DX11 ImGui Overlay Demo (Dead Cells)

A simple internal overlay demo for DirectX11 using **Kiero** + **ImGui**, with basic memory manipulation
Designed purely for **learning purposes**, to understand how internal overlays work

> ⚠️ **Important:** This is intended for single-player Dead Cells only

---

## Features

- DX11 `Present` hook via [Kiero](https://github.com/Rebzzel/kiero)  
- ImGui menu toggle with `HOME` key  
- Read/Write process memory (RPM/WPM)  
- Multi-level pointers (DMA)  
- Pattern scanning with masks  
- Separate thread for value updates (avoids spamming/CRASH)  
- Clean shutdown and resource release on DLL unload  

---

## Build Instructions

1. Clone the repository  
2. Add **Kiero** and **ImGui** (DX11 + Win32 backend)
3. Build as **x86 DLL** in Visual Studio
4. Inject into Dead Cells using any DLL injector

---

## Purpose

This project is intended to help you:

- Understand how internal overlays work  
- Hook DirectX11 safely  
- Read and write memory of another process  
- Avoid crashing the game while experimenting  

This is strictly for educational purposes and personal experiments  

---

## Credits

- **Kiero** — DX11 hook  
- **ImGui** — GUI menu  
- **Guided Hacking forums** — general ideas and references  

---

## License

MIT — use freely, but not for online cheating