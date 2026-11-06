#pragma once
#include "prism/abi.hpp"
#if !PRISM_HAS_X64_GUEST_ABI
#error "Prism guest invocation requires an x86-64 host process"
#endif
namespace prism::abi {
// Caller owns validation and lifetime. Never pass a null/unmapped entry.
std::uint32_t invoke_entry(win::EntryPoint entry) noexcept;
win::LRESULT invoke_window_proc(win::WindowProc proc, win::HWND window,
                               win::DWORD message, win::WPARAM wparam,
                               win::LPARAM lparam) noexcept;
}
