#include "includes.h"
#include "offsets.h"
#include <list>

#ifdef _WIN64
#define GWL_WNDPROC GWLP_WNDPROC
#endif

using namespace std;
using namespace hazedumper;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EndScene oEndScene = NULL;
WNDPROC oWndProc;
static HWND window = NULL;

void InitImGui(LPDIRECT3DDEVICE9 pDevice)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(pDevice);
}

bool bhop = false;

bool esp = false;

bool glow = false;
bool glowTeam = false;
ImColor enemyGlowColor;
ImColor teamGlowColor;

bool isOpen = true;
int tabPosition = 1;
bool init = false;
long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (!init)
	{
		InitImGui(pDevice);
		init = true;
	}

	if (GetAsyncKeyState(VK_INSERT) & 1) {
		isOpen = !isOpen;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (isOpen){
		ImGui::Begin("Moonlight");

		if (ImGui::Button("Visuals", ImVec2(90, 25))) {
			tabPosition = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button("Aimbot", ImVec2(90, 25))) {
			tabPosition = 2;
		}
		ImGui::SameLine();
		if (ImGui::Button("Misc", ImVec2(90, 25))) {
			tabPosition = 3;
		}
		ImGui::SameLine();
		if (ImGui::Button("Config", ImVec2(90, 25))) {
			tabPosition = 4;
		}

		if (tabPosition == 1) {
			ImGui::Text("Glow");
			ImGui::Checkbox("Enable", &glow);
			ImGui::Checkbox("Enable for team", &glowTeam);
			ImGui::ColorEdit4("Team", (float*)&teamGlowColor);
			ImGui::ColorEdit4("Enemy", (float*)&enemyGlowColor);
			ImGui::Dummy(ImVec2(0, 20));

			ImGui::Text("ESP");
			ImGui::Checkbox("Enable", &esp);
		}

		if (tabPosition == 3) {
			ImGui::Checkbox("Bhop", &bhop);
		}
		
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE; // skip to next window

	window = handle;
	return FALSE; // window found abort search
}

HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool attached = false;
	{
		if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
		{
			kiero::bind(42, (void**)& oEndScene, hkEndScene);
			do
				window = GetProcessWindow();
			while (window == NULL);
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
			attached = true;
		}
	} while (!attached);
	return TRUE;
}

DWORD WINAPI BhopThread(LPVOID lp) {
	DWORD gameClient = (DWORD)GetModuleHandle("client.dll");

	while (true) {
		if (bhop) {
			DWORD localPlayer = *(DWORD*)(gameClient + signatures::dwLocalPlayer);

			if (localPlayer != NULL) {
				if (GetAsyncKeyState(VK_SPACE)) {
					DWORD playerFlag = *(BYTE*)(localPlayer + netvars::m_fFlags);

					if (playerFlag & (1 << 0)) {
						*(DWORD*)(gameClient + signatures::dwForceJump) = 6;
					}
				}
			}
		}
	}
};

DWORD WINAPI GlowThread(LPVOID lp) {
	DWORD gameClient = (DWORD)GetModuleHandle("client.dll");

	while (true) {
		if (glow) {
			DWORD localPlayer = *(DWORD*)(gameClient + signatures::dwLocalPlayer);
			DWORD glowManager = *(DWORD*)(gameClient + signatures::dwGlowObjectManager);

			if (localPlayer != NULL) {
				int localPlayerTeam = *(int*)(localPlayer + netvars::m_iTeamNum);

				for (int i = 0; i < 32; i++) {
					DWORD entity = *(DWORD*)(gameClient + signatures::dwEntityList + i * 0x10);

					if (entity != NULL) {
						DWORD entityGlow = *(DWORD*)(entity + netvars::m_iGlowIndex);
						int entityTeam = *(int*)(entity + netvars::m_iTeamNum);

						if (localPlayerTeam == entityTeam) {
							if (glowTeam) {
								*(float*)(glowManager + entityGlow * 0x38 + 0x8) = teamGlowColor.Value.x;
								*(float*)(glowManager + entityGlow * 0x38 + 0xC) = teamGlowColor.Value.y;
								*(float*)(glowManager + entityGlow * 0x38 + 0x10) = teamGlowColor.Value.z;
								*(float*)(glowManager + entityGlow * 0x38 + 0x14) = teamGlowColor.Value.w;

								*(bool*)(glowManager + entityGlow * 0x38 + 0x28) = true;
							}
						}
						else {
							*(float*)(glowManager + entityGlow * 0x38 + 0x8) = enemyGlowColor.Value.x;
							*(float*)(glowManager + entityGlow * 0x38 + 0xC) = enemyGlowColor.Value.y;
							*(float*)(glowManager + entityGlow * 0x38 + 0x10) = enemyGlowColor.Value.z;
							*(float*)(glowManager + entityGlow * 0x38 + 0x14) = enemyGlowColor.Value.w;

							*(bool*)(glowManager + entityGlow * 0x38 + 0x28) = true;
						}
					}
				}
			}
		}
	}
};


BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, BhopThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, GlowThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}
