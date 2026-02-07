#include "interfaces.hpp"
#include "../sdk/memory_utils.hpp"
#include <stdexcept>
#include <format>
#include <Windows.h>

namespace interfaces {
	template <class T>
	T* capture_interface(const char* module_name, const char* interface_name) {
		const HMODULE module_handle = GetModuleHandleA(module_name);
		if (module_handle == nullptr) {
			throw std::runtime_error(
				std::format("failed to get handle for module \"{}\"", module_name));
		}

		using create_interface_fn = T* (*)(const char*, int*);
		const auto create_interface =
			reinterpret_cast<create_interface_fn>(GetProcAddress(module_handle, "CreateInterface"));
		if (create_interface == nullptr) {
			throw std::runtime_error(std::format("failed to capture interface \"{}\" from \"{}\"",
												 interface_name, module_name));
		}

		return create_interface(interface_name, nullptr);
	}

	static void create_d3d11_resources() {
		{
			std::uint8_t* address =
				sdk::find_pattern("rendersystemdx11.dll", "48 89 2D ? ? ? ? 48 C7 05");

			swap_chain_dx11 = **reinterpret_cast<sdk::interface_swap_chain_dx11***>(
				sdk::resolve_absolute_rip_address(address, 3, 7));
			if (swap_chain_dx11 == nullptr) {
				throw std::runtime_error("failed to capture interface_swap_chain_dx11");
			}
		}

		if (swap_chain_dx11->swap_chain == nullptr) {
			throw std::runtime_error("swap_chain_dx11->swap_chain is null, padding may be outdated.");
		}

		IDXGISwapChain* swap_chain = swap_chain_dx11->swap_chain;

		if (FAILED(swap_chain->GetDevice(__uuidof(ID3D11Device),
										reinterpret_cast<void**>(&d3d11_device)))) {
			throw std::runtime_error("failed to get d3d11 device from swap chain");
		}

		if (d3d11_device == nullptr) {
			throw std::runtime_error("d3d11 device is null");

		}

		d3d11_device->GetImmediateContext(&d3d11_device_context);

		if (d3d11_device_context == nullptr) {
			throw std::runtime_error("d3d11 device context is null");
		}

		{
			DXGI_SWAP_CHAIN_DESC swap_chain_desc;
			if (FAILED(swap_chain->GetDesc(&swap_chain_desc))) {
				throw std::runtime_error("failed to get swap chain description");
			}

			hwnd = swap_chain_desc.OutputWindow;

			if (hwnd == nullptr) {
				throw std::runtime_error("swap chain hwnd is null");
			}
		}

		create_render_target();
	}

	void create() {
		{
			std::uint8_t* address =
				sdk::find_pattern("client.dll", "48 8B 0D ? ? ? ? 4C 8D 8F ? ? ? ? 45 33 F6");

			csgo_input = *reinterpret_cast<sdk::interface_csgo_input**>(
				sdk::resolve_absolute_rip_address(address, 3, 7));
			if (csgo_input == nullptr) {
				throw std::runtime_error("failed to capture interface_csgo_input");
			}
		}

		input_system = capture_interface<sdk::interface_input_system>("inputsystem.dll",
																	  "InputSystemVersion001");

		create_d3d11_resources();
	}
	void destroy() {
		destroy_render_target();

		if (d3d11_device_context != nullptr) {
			d3d11_device_context->Release();
		}

		if (d3d11_device != nullptr) {
			d3d11_device->Release();
		}
		swap_chain_dx11 = nullptr;
		input_system = nullptr;
		csgo_input = nullptr;
		hwnd = nullptr;
	}

	void create_render_target() {
		if (!d3d11_device || !swap_chain_dx11 || !swap_chain_dx11->swap_chain) {
			throw std::runtime_error("d3d11 device or swap chain is null");
		}

		ID3D11Texture2D* back_buffer = nullptr;
		if (FAILED(swap_chain_dx11->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
										 reinterpret_cast<void**>(&back_buffer)))) {
			throw std::runtime_error("failed to get back buffer from swap chain");
		}

		if (back_buffer == nullptr) {
			throw std::runtime_error("back buffer is null");
		}

		if (FAILED(d3d11_device->CreateRenderTargetView(back_buffer, nullptr,
														&d3d11_render_target_view))) {
			back_buffer->Release();
			throw std::runtime_error("failed to create render target view from back buffer");
		}

		back_buffer->Release();

		if (d3d11_render_target_view == nullptr) {
			throw std::runtime_error("render target view is null");
		}
	}

	void destroy_render_target() {
		if (d3d11_render_target_view != nullptr) {
			d3d11_render_target_view->Release();
			d3d11_render_target_view = nullptr;
		}
	}
}