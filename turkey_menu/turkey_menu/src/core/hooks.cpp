#include <Windows.h>
#include "../../external/minhook/MinHook.h"
#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_dx11.h"
#include "../../external/imgui/imgui_impl_win32.h"
#include "hooks.hpp"
#include <stdexcept>
#include "menu.hpp"
#include "globals.hpp"

HRESULT __stdcall hook_present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
	// do stuff here

	if (!interfaces::d3d11_render_target_view) {
		interfaces::create_render_target();
	}

	if (interfaces::d3d11_device_context) {
		interfaces::d3d11_device_context->OMSetRenderTargets(
			1, &interfaces::d3d11_render_target_view, nullptr);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	menu::render();
	// visuals::render

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return hooks::original_present(swap_chain, sync_interval, flags);
}

HRESULT __stdcall hook_resize_buffers(IDXGISwapChain* swap_chain, UINT buffer_count, UINT width,
									  UINT height, DXGI_FORMAT new_format, UINT swap_chain_flags) {
	const HRESULT result = hooks::original_resize_buffers(swap_chain, buffer_count , width, height,
														 new_format, swap_chain_flags);
	if (SUCCEEDED(result)) {
		interfaces::destroy_render_target();
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	return result;
}

HRESULT __stdcall hook_create_swap_chain(IDXGIFactory* factory, IUnknown* device,
										   DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
										   IDXGISwapChain** swap_chain) {

	ImGui_ImplDX11_InvalidateDeviceObjects();
	interfaces::destroy_render_target();

	return hooks::original_create_swap_chain(factory, device, swap_chain_desc, swap_chain);
}

bool __fastcall hook_mouse_input_enabled(sdk::interface_csgo_input* csgo_input) {
	return globals::menu_opened ? false : hooks::original_mouse_input_enabled(csgo_input);
}

void* __fastcall hook_set_relative_mouse_mode(sdk::interface_input_system* input_system,
	bool enabled) {
	globals::relative_mouse_mode = enabled;
	return hooks::original_set_relative_mouse_mode(input_system,
												   globals::menu_opened ? false : enabled);
}
namespace hooks {
	void create() {
		if (MH_Initialize() != MH_OK) {
			throw std::runtime_error("failed to initialize MinHook.");
		}

		// place all hooks

		if (MH_CreateHook(
			sdk::virtual_function_get<void*, 8>(interfaces::swap_chain_dx11->swap_chain),
			&hook_present, reinterpret_cast<void**>(&original_present)) != MH_OK) {
			throw std::runtime_error("failed to create hook for IDXGISwapChain::Present.");
		}

		if (MH_CreateHook(
			sdk::virtual_function_get<void*, 13>(interfaces::swap_chain_dx11->swap_chain),
			&hook_resize_buffers, reinterpret_cast<void**>(&original_resize_buffers)) != MH_OK) {
			throw std::runtime_error("failed to create hook for IDXGISwapChain resize_buffers.");
		}

		{
			IDXGIDevice* dxgi_device = nullptr;
			if (FAILED(interfaces::d3d11_device->QueryInterface(
					__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device)))) {
				throw std::runtime_error("failed to get dxgi device from d3d11 device");
			}

			IDXGIAdapter* dxgi_adapter = nullptr;
			if (FAILED(dxgi_device->GetAdapter(&dxgi_adapter))) {
				dxgi_device->Release();
				throw std::runtime_error("failed to get dxgi adapter from dxgi device");
			}

			IDXGIFactory* dxgi_factory = nullptr;
			if (FAILED(dxgi_adapter->GetParent(__uuidof(IDXGIFactory),
											   reinterpret_cast<void**>(&dxgi_factory)))) {
				dxgi_adapter->Release();
				dxgi_device->Release();
				throw std::runtime_error("failed to get dxgi factory from dxgi adapter");
			}

			if (MH_CreateHook(sdk::virtual_function_get<void*, 10>(dxgi_factory),
							  &hook_create_swap_chain,
							  reinterpret_cast<void**>(&original_create_swap_chain)) != MH_OK) {
				dxgi_factory->Release();
				dxgi_adapter->Release();
				dxgi_device->Release();
				throw std::runtime_error("failed to create swap chain hook");
			}

			dxgi_factory->Release();
			dxgi_adapter->Release();
			dxgi_device->Release();
		}

		if (MH_CreateHook(sdk::virtual_function_get<void*, 15>(interfaces::csgo_input),
						  &hook_mouse_input_enabled,
						  reinterpret_cast<void**>(&original_mouse_input_enabled)) != MH_OK) {
			throw std::runtime_error("failed to create mouse input enabled hook");
		}

		if (MH_CreateHook(sdk::virtual_function_get<void*, 76>(interfaces::input_system),
						  &hook_set_relative_mouse_mode,
						  reinterpret_cast<void**>(&original_set_relative_mouse_mode)) != MH_OK) {
			throw std::runtime_error("failed to create set relative mouse mode hook");
		}

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
			throw std::runtime_error("failed to enable MinHook hooks.");
		}
	}

	void destroy() {
		MH_DisableHook(MH_ALL_HOOKS);
		MH_RemoveHook(MH_ALL_HOOKS);

		MH_Uninitialize();
	}
}