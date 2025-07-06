#include "util/includes.h"
#include "hack.h"
#include "unload.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool menuOpen = false;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

void CleanupImGui()
{
	if (oWndProc) {
		SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (mainRenderTargetView) {
		mainRenderTargetView->Release();
		mainRenderTargetView = nullptr;
	}
	if (pContext) {
		pContext->Release();
		pContext = nullptr;
	}
	if (pDevice) {
		pDevice->Release();
		pDevice = nullptr;
	}
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
		menuOpen = !menuOpen;

		ImGuiIO& io = ImGui::GetIO();
		io.MouseDrawCursor = menuOpen;
		io.WantCaptureMouse = menuOpen;
		io.WantCaptureKeyboard = menuOpen;
		return 0; // Block input further
	}

	if (menuOpen) {
		// Give ImGui input priority
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
			return 1;
		}

		// Block keyboard and mouse input to the game while menu open
		if ((uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) ||
			(uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && uMsg != WM_MOUSEWHEEL && uMsg != WM_MOUSEHWHEEL)) {
			return 0;
		}

		// Let ImGui handle mouse wheel scroll
		if (uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEHWHEEL) {
			return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		}

		// Block system commands like alt-tab to avoid issues
		if (uMsg == WM_SYSCOMMAND) {
			return 0;
		}
	}

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (shouldExit) {
		return oPresent(pSwapChain, SyncInterval, Flags);
	}

	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}
		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImVec2 windowSize = ImGui::GetIO().DisplaySize;
	ImGui::GetIO().FontGlobalScale = max(1.f, ((windowSize.x / 1920.0f) * 0.85f));

	if (menuOpen) {
		ImGui::GetBackgroundDrawList()->AddRectFilled(
			ImVec2(0, 0),
			ImGui::GetIO().DisplaySize,
			IM_COL32(0, 0, 0, 150)
		);

		ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::Checkbox("Enabled", &Aimbot::enabled);
			ImGui::SliderFloat("FOV", &Aimbot::fov, 1.f, 180.f);
			ImGui::SliderFloat("Smoothing", &Aimbot::smoothing, 1.f, 30.f);
		}
		ImGui::End();
	}

	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI ImGuiThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);

	// Wait for exit signal
	while (!shouldExit) {
		Sleep(50);
	}

	// Cleanup
	CleanupImGui();
	kiero::shutdown();

	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, ImGuiThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, HackThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, UnloadThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		Sleep(100);
		break;
	}
	return TRUE;
}