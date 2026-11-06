#include "bridge.hpp"
namespace prism::abi {
std::uint32_t invoke_entry(win::EntryPoint entry) noexcept { return entry(); }
win::LRESULT invoke_window_proc(win::WindowProc proc, win::HWND window,
                               win::DWORD message, win::WPARAM wparam,
                               win::LPARAM lparam) noexcept {
  return proc(window, message, wparam, lparam);
}
}
