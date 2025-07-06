#pragma once
#include "util/mem.h"
#include "unload.h"
#include "hacks/aimbot.h"

static DWORD WINAPI HackThread(LPVOID lpParam) {
    mem.loadModules();
    while (!shouldExit) {
        Aimbot::RunOnce();
        Sleep(10);
    }
    FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
}
