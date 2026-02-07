#include "menu.hpp"
#include "globals.hpp"
#include "interfaces.hpp"
#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_dx11.h"
#include "../../external/imgui/imgui_impl_win32.h"
#include <stdexcept>
#include "hooks.hpp"

static WNDPROC original_wndproc = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall hook_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_KEYDOWN && wparam == VK_INSERT) {
		globals::menu_opened = !globals::menu_opened;

		hooks::original_set_relative_mouse_mode(
			interfaces::input_system, globals::menu_opened ? false : globals::relative_mouse_mode);
		// ImGui::GetIO().MouseDrawCursor = globals::menu_opened;
	}

	if (globals::menu_opened) {
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
		return true;
	} else {
		return CallWindowProc(original_wndproc, hwnd, msg, wparam, lparam);
	}
}

namespace menu {
	void create() {
		if (!interfaces::d3d11_device || !interfaces::d3d11_device_context || !interfaces::hwnd) {
			throw std::runtime_error("interfaces not initialized");
		}

		original_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
			interfaces::hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hook_wndproc)));

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		if (!ImGui_ImplWin32_Init(interfaces::hwnd)) {
			throw std::runtime_error("failed to initialize imgui win32 implementation");
		}

		if (!ImGui_ImplDX11_Init(interfaces::d3d11_device, interfaces::d3d11_device_context)) {
			throw std::runtime_error("failed to initialize imgui dx11 implementation");
		}
	}

	void destroy() {
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		if (original_wndproc) {
			SetWindowLongPtrA(interfaces::hwnd, GWLP_WNDPROC,
							  reinterpret_cast<LONG_PTR>(original_wndproc));
			original_wndproc = nullptr;
		}
	}

	void render() {
		if (!globals::menu_opened) {
			return;
		}

		ImGui::Begin("Turkey Menu");
		ImGui::Text("Hello from Turkey Menu!");
		ImGui::End();
	}
}