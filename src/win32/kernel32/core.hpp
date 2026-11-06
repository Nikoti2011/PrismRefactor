#pragma once
#include "prism/abi.hpp"
#include "runtime/runtime.hpp"
namespace prism::kernel32 {
inline constexpr WindowsError invalid_parameter = 87;
inline constexpr WindowsError module_not_found = 126;
inline constexpr WindowsError general_failure = 31;
// Native service layer is testable on ARM64. Pointer arguments must be valid
// accessible guest memory in the x64 worker; null is handled explicitly.
GuestAddress module_handle(GuestProcessContext&, GuestThreadContext&, const win::WCHAR*) noexcept;
win::BOOL counter(GuestThreadContext&, win::LARGE_INTEGER*) noexcept;
win::BOOL frequency(GuestThreadContext&, win::LARGE_INTEGER*) noexcept;
void sleep(GuestThreadContext&, win::DWORD milliseconds) noexcept;
void debug_output(GuestThreadContext&, const char*) noexcept;
[[noreturn]] void exit_process(win::DWORD code) noexcept;
// Atomic registration: duplicate or failed registration leaves registry intact.
Status register_core(runtime::Registry&);
#if PRISM_HAS_X64_GUEST_ABI
namespace adapters {
win::HANDLE PRISM_GUEST_ABI GetModuleHandleW(const win::WCHAR*) noexcept;
win::BOOL PRISM_GUEST_ABI QueryPerformanceCounter(win::LARGE_INTEGER*) noexcept;
win::BOOL PRISM_GUEST_ABI QueryPerformanceFrequency(win::LARGE_INTEGER*) noexcept;
win::DWORD PRISM_GUEST_ABI GetLastError() noexcept;
void PRISM_GUEST_ABI SetLastError(win::DWORD) noexcept;
void PRISM_GUEST_ABI Sleep(win::DWORD) noexcept;
void PRISM_GUEST_ABI OutputDebugStringA(const char*) noexcept;
[[noreturn]] void PRISM_GUEST_ABI ExitProcess(win::DWORD) noexcept;
}
#endif
}
