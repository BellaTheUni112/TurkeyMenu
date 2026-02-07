#include <Windows.h>
#include "../../external/minhook/MinHook.h"
#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_dx11.h"
#include "../../external/imgui/imgui_impl_win32.h"
#include "hooks.hpp"
#include <stdexcept>

HRESULT __stdcall hook_present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
	return hooks::original_present(interfaces::swap_chain_dx11->swap_chain, sync_interval, flags);
}

HRESULT __stdcall hook_resize_buffers(IDXGISwapChain* swap_chain, UINT buffer_count, UINT width,
	UINT height, DXGI_FORMAT new_format, UINT swap_chain_flags) {
	const HRESULT result = hooks::original_resize_buffers(swap_chain, buffer_count, width, height,
		new_format, swap_chain_flags);
	return result;
}

HRESULT __stdcall hook_create_swaop_chain(IDXGIFactory* factory, IUnknown* device,
	DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
	IDXGISwapChain** swap_chain) {
	return hooks::original_create_swap_chain(factory, device, swap_chain_desc, swap_chain);
}

namespace hooks {
	void create() {
		if (MH_Initialize() != MH_OK) {
			throw std::runtime_error("failed to initialize MinHook.");
		}

		// place all hooks

		if (MH_CreateHook(
			sdk::virtual_function_get<void*, 8>(interfaces::swap_chain_dx11->swap_chain),
			&hook_present, reinterpret_cast<void**>(original_present)) != MH_OK) {
			throw std::runtime_error("failed to create hook for IDXGISwapChain::Present.");
		}

		if (MH_CreateHook(
			sdk::virtual_function_get<void*, 13>(interfaces::swap_chain_dx11->swap_chain),
			&hook_resize_buffers, reinterpret_cast<void**>(original_resize_buffers)) != MH_OK) {
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
				reinterpret_cast<void**>(original_create_swap_chain)) != MH_OK{
  dx_factory->Release();
  dxgi_adapter->Release();
  dxgi_device->Release();
  throw std::runtime_error("failed to create swap chain hook");
				}
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