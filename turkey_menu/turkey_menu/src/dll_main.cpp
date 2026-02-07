#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdexcept>

#include "core/interfaces.hpp"
#include "core/hooks.hpp"
#include "core/menu.hpp"

DWORD WINAPI cheat_thread(LPVOID instance) {
	MessageBox(nullptr, L"Hello", L"World", MB_ICONHAND);

	try {
		interfaces::create();
		menu::create();
		hooks::create();
	} catch (const std::exception& e) {
		hooks::destroy();
		menu::destroy();
		interfaces::destroy();
		MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
		FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(instance), 1);
	}

	while (!(GetAsyncKeyState(VK_END) & 1)) {
		Sleep(100);
	}

	hooks::destroy();
	menu::destroy();
	interfaces::destroy();

	MessageBoxA(nullptr, "TurkeyMenu unloading...", "Info", MB_ICONINFORMATION);

	FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(instance), 0);

	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);

		HANDLE thread = CreateThread(nullptr, 0, cheat_thread, instance, 0, nullptr);
		if (thread != NULL) {

			CloseHandle(thread);

			return TRUE;
		}

		// If thread creation failed, fail DLL load
		return FALSE;
	}

	// For other reasons, return TRUE
	return TRUE;
}