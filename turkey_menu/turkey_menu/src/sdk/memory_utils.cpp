#include "memory_utils.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // !WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>
#include <stdexcept>
#include <format>

namespace sdk {
    static std::vector<int> ida_pattern_to_bytes(const char* pattern) {
        std::vector<int> bytes = {};

        char* start = const_cast<char*>(pattern);
        const char* end = const_cast<char*>(pattern) + std::strlen(pattern);

        for (char* current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;

                if (*current == '?') {
                    ++current;
                }

                bytes.push_back(-1);
            } else {
                bytes.push_back(std::strtoul(current, &current, 16));
            }
        }

        return bytes;
    }

    std::uint8_t* find_pattern(const char* module_name, const char* pattern) {
        const HMODULE module_handle = GetModuleHandleA(module_name);
        if (module_handle == nullptr) {
            throw std::runtime_error(
                std::format("failed to get handle for module \"{}\"", module_name));
        }

        const auto* dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
        const auto* nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<std::uintptr_t*>(module_handle) + dos_header->e_lfanew);

        const DWORD image_size = nt_headers->OptionalHeader.SizeOfImage;
        auto* image_data = reinterpret_cast<std::uint8_t*>(module_handle);

        const std::vector<int> bytes = ida_pattern_to_bytes(pattern);
        const std::size_t pattern_size = bytes.size();
        const int* pattern_data = bytes.data();

        for (std::size_t i = 0ul; i < image_size - pattern_size; ++i) {
            bool found = true;

            for (std::size_t j = 0ul; j < pattern_size; ++j) {
                if (image_data[i + j] != pattern_data[j] && pattern_data[j] != -1) {
                    found = false;
                    break;
                }
            }

            if (found == true) {
                return &image_data[i];
            }
        }

        throw std::runtime_error(
            std::format("failed to find pattern \"{}\" in module \"{}\"", pattern, module_name));
    }

    std::uint8_t* resolve_absolute_rip_address(std::uint8_t* instruction,
                                               std::size_t offset_to_displacement,
                                               std::size_t instruction_size) {
        auto displacement = *reinterpret_cast<const std::int32_t*>(
            reinterpret_cast<std::uintptr_t>(instruction) + offset_to_displacement);
        return instruction + instruction_size + displacement;
    }
}  // namespace sdk