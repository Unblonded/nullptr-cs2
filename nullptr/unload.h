#pragma once
#include <windows.h>

static bool shouldExit = false;

static DWORD WINAPI UnloadThread(LPVOID lp) {
	while (true) {
		if (GetAsyncKeyState(VK_END) & 0x8000) {
			shouldExit = true;
			break;
		}
	}
	FreeLibraryAndExitThread(static_cast<HMODULE>(lp), 0);
}