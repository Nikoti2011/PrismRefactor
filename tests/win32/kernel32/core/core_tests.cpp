#include "win32/kernel32/core.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <string_view>
#include <thread>
#include <pthread.h>
using namespace prism;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); ++checks; } while (0)
#if PRISM_HAS_X64_GUEST_ABI
template<class Fn> Fn imported(runtime::Registry& r, const char* name) {
  return reinterpret_cast<Fn>(std::get<GuestAddress>(r.resolve("kernel32.dll", name)));
}
#endif
static volatile sig_atomic_t signals_seen = 0;
static void interrupt_sleep(int) { signals_seen = 1; }
int main(int argc, char** argv) {
  int checks = 0;
  GuestProcessContext process{0x180000000}; GuestThreadContext thread{7, 1234};
  runtime::ContextScope scope(process, thread);
  try {
    if (argc > 1) {
      if (std::string_view(argv[1]) == "--exit") {
#if PRISM_HAS_X64_GUEST_ABI
        kernel32::adapters::ExitProcess(0x123456ab);
#else
        kernel32::exit_process(0x123456ab);
#endif
      }
      if (std::string_view(argv[1]) == "--debug") {
#if PRISM_HAS_X64_GUEST_ABI
        kernel32::adapters::OutputDebugStringA("fixture diagnostic");
#else
        kernel32::debug_output(thread, "fixture diagnostic");
#endif
        return thread.last_error == 1234 ? 0 : 1;
      }
      return 2;
    }
    CHECK(kernel32::module_handle(process, thread, nullptr) == process.image_base);
    CHECK(thread.last_error == 1234);
    CHECK(!kernel32::module_handle(process, thread, u"kernel32.dll") && thread.last_error == 126);
    win::LARGE_INTEGER freq{}, before{}, after{};
    CHECK(!kernel32::frequency(thread, nullptr) && thread.last_error == 87);
    CHECK(!kernel32::counter(thread, nullptr) && thread.last_error == 87);
    kernel32::debug_output(thread, nullptr); CHECK(thread.last_error == 87);
    thread.last_error = 0; kernel32::sleep(thread, UINT32_MAX); CHECK(thread.last_error == 87);
    thread.last_error = 1234;
    CHECK(kernel32::frequency(thread, &freq) && freq.QuadPart == 1000000000);
    auto host_before = std::chrono::steady_clock::now();
    CHECK(kernel32::counter(thread, &before));
    kernel32::sleep(thread, 25);
    CHECK(kernel32::counter(thread, &after));
    const auto elapsed = after.QuadPart - before.QuadPart;
    auto host_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - host_before).count();
    CHECK(elapsed >= 25000000 && host_elapsed >= elapsed && host_elapsed - elapsed < 100000000);
    CHECK(thread.last_error == 1234);
    for (int i = 0; i < 1000; ++i) {
      before = after; CHECK(kernel32::counter(thread, &after) && after.QuadPart >= before.QuadPart);
    }
    kernel32::sleep(thread, 0); CHECK(thread.last_error == 1234);
    struct sigaction action{}, previous{}; action.sa_handler = interrupt_sleep;
    sigemptyset(&action.sa_mask); CHECK(sigaction(SIGUSR1, &action, &previous) == 0);
    const auto sleeper = pthread_self();
    std::thread interrupter([sleeper] { std::this_thread::sleep_for(std::chrono::milliseconds(10)); pthread_kill(sleeper, SIGUSR1); });
    kernel32::counter(thread, &before); kernel32::sleep(thread, 60); kernel32::counter(thread, &after);
    interrupter.join(); sigaction(SIGUSR1, &previous, nullptr);
    CHECK(signals_seen && after.QuadPart - before.QuadPart >= 60000000 && thread.last_error == 1234);
    runtime::Registry registry;
#if PRISM_HAS_X64_GUEST_ABI
    CHECK(std::holds_alternative<std::monostate>(kernel32::register_core(registry)));
    using Set = void (PRISM_GUEST_ABI *)(win::DWORD) noexcept;
    using Get = win::DWORD (PRISM_GUEST_ABI *)() noexcept;
    using Module = win::HANDLE (PRISM_GUEST_ABI *)(const win::WCHAR*) noexcept;
    using Clock = win::BOOL (PRISM_GUEST_ABI *)(win::LARGE_INTEGER*) noexcept;
    using Debug = void (PRISM_GUEST_ABI *)(const char*) noexcept;
    imported<Set>(registry, "SetLastError")(4321);
    CHECK(imported<Get>(registry, "GetLastError")() == 4321);
    CHECK(imported<Module>(registry, "GetModuleHandleW")(nullptr) == process.image_base);
    CHECK(imported<Clock>(registry, "QueryPerformanceFrequency")(&freq) && freq.QuadPart == 1000000000);
    CHECK(imported<Clock>(registry, "QueryPerformanceCounter")(&before));
    imported<Set>(registry, "Sleep")(2);
    imported<Debug>(registry, "OutputDebugStringA")("ABI diagnostic");
    CHECK(imported<Get>(registry, "GetLastError")() == 4321);
    CHECK(std::holds_alternative<Diagnostic>(kernel32::register_core(registry)));
    runtime::Registry collision; collision.add("kernel32.dll", "Sleep", 1);
    CHECK(std::holds_alternative<Diagnostic>(kernel32::register_core(collision)));
    CHECK(std::holds_alternative<Diagnostic>(collision.resolve("kernel32.dll", "GetModuleHandleW")));
    std::cout << "x64 resolved ABI calls PASS\n";
#else
    CHECK(std::get<Diagnostic>(kernel32::register_core(registry)).code == ErrorCode::execution_unavailable);
    CHECK(std::holds_alternative<Diagnostic>(registry.resolve("kernel32.dll", "Sleep")));
    std::cout << "ARM64 adapter registration rejected as required\n";
#endif
    CHECK(std::holds_alternative<Diagnostic>(registry.resolve("kernel32.dll", "CreateFileW")));
    std::cout << "PASS core checks=" << checks << " sleep_ns=" << elapsed << '\n';
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
