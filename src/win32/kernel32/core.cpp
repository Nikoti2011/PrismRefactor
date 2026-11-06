#include "core.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sched.h>

namespace prism::kernel32 {
GuestAddress module_handle(GuestProcessContext& process, GuestThreadContext& thread,
                           const win::WCHAR* name) noexcept {
  if (name || !process.image_base) { thread.last_error = module_not_found; return 0; }
  return process.image_base;
}
win::BOOL frequency(GuestThreadContext& thread, win::LARGE_INTEGER* out) noexcept {
  if (!out) { thread.last_error = invalid_parameter; return 0; }
  out->QuadPart = 1000000000; return 1;
}
win::BOOL counter(GuestThreadContext& thread, win::LARGE_INTEGER* out) noexcept {
  if (!out) { thread.last_error = invalid_parameter; return 0; }
  timespec now{};
  if (clock_gettime(CLOCK_MONOTONIC, &now) || now.tv_sec < 0 ||
      now.tv_sec > (std::numeric_limits<std::int64_t>::max() - now.tv_nsec) / 1000000000) {
    thread.last_error = general_failure; return 0;
  }
  out->QuadPart = static_cast<std::int64_t>(now.tv_sec) * 1000000000 + now.tv_nsec;
  return 1;
}
void sleep(GuestThreadContext& thread, win::DWORD milliseconds) noexcept {
  if (milliseconds == UINT32_MAX) { thread.last_error = invalid_parameter; return; }
  if (!milliseconds) { if (sched_yield()) thread.last_error = general_failure; return; }
  timespec remaining{static_cast<time_t>(milliseconds / 1000),
                     static_cast<long>(milliseconds % 1000) * 1000000};
  while (nanosleep(&remaining, &remaining)) {
    if (errno != EINTR) { thread.last_error = general_failure; return; }
  }
}
void debug_output(GuestThreadContext& thread, const char* message) noexcept {
  if (!message) { thread.last_error = invalid_parameter; return; }
  // Serialize complete messages from concurrent workers. No allocation/throw.
  flockfile(stderr);
  const bool failed = std::fputs("[prism guest] ", stderr) < 0 ||
                      std::fputs(message, stderr) < 0 || std::fputc('\n', stderr) == EOF;
  funlockfile(stderr);
  if (failed) thread.last_error = general_failure;
}
[[noreturn]] void exit_process(win::DWORD code) noexcept {
  // Report the complete Windows DWORD; POSIX wait status carries only low 8 bits.
  std::fprintf(stderr, "[prism guest] ExitProcess code=%u\n", code);
  std::fflush(stderr);
  std::_Exit(static_cast<int>(code & 255));
}
#if PRISM_HAS_X64_GUEST_ABI
namespace {
runtime::Binding required_context() noexcept {
  auto b = runtime::current_context();
  if (!b.process || !b.thread) {
    std::fputs("[prism] kernel adapter called without guest context\n", stderr);
    std::abort();
  }
  return b;
}
}
namespace adapters {
win::HANDLE PRISM_GUEST_ABI GetModuleHandleW(const win::WCHAR* name) noexcept {
  auto b = required_context(); return module_handle(*b.process, *b.thread, name);
}
win::BOOL PRISM_GUEST_ABI QueryPerformanceCounter(win::LARGE_INTEGER* out) noexcept {
  return counter(*required_context().thread, out);
}
win::BOOL PRISM_GUEST_ABI QueryPerformanceFrequency(win::LARGE_INTEGER* out) noexcept {
  return frequency(*required_context().thread, out);
}
win::DWORD PRISM_GUEST_ABI GetLastError() noexcept { return required_context().thread->last_error; }
void PRISM_GUEST_ABI SetLastError(win::DWORD value) noexcept { required_context().thread->last_error = value; }
void PRISM_GUEST_ABI Sleep(win::DWORD ms) noexcept { sleep(*required_context().thread, ms); }
void PRISM_GUEST_ABI OutputDebugStringA(const char* text) noexcept { debug_output(*required_context().thread, text); }
[[noreturn]] void PRISM_GUEST_ABI ExitProcess(win::DWORD code) noexcept {
  (void)required_context(); exit_process(code);
}
}
#endif
Status register_core(runtime::Registry& registry) {
#if PRISM_HAS_X64_GUEST_ABI
  auto staged = registry;
#define PRISM_REGISTER(name) \
  { auto result = staged.add("KERNEL32.DLL", #name, reinterpret_cast<GuestAddress>(&adapters::name)); \
    if (auto* error = std::get_if<Diagnostic>(&result)) return *error; }
  PRISM_REGISTER(GetModuleHandleW)
  PRISM_REGISTER(QueryPerformanceCounter)
  PRISM_REGISTER(QueryPerformanceFrequency)
  PRISM_REGISTER(GetLastError)
  PRISM_REGISTER(SetLastError)
  PRISM_REGISTER(Sleep)
  PRISM_REGISTER(OutputDebugStringA)
  PRISM_REGISTER(ExitProcess)
#undef PRISM_REGISTER
  registry = std::move(staged);
  return std::monostate{};
#else
  (void)registry;
  Diagnostic d; d.code = ErrorCode::execution_unavailable; d.event = "register_core";
  d.unsupported_reason = "x86-64 process required for Windows x64 adapters"; return d;
#endif
}
}
