#include "runtime/runtime.hpp"
#include <iostream>
#include <thread>
#include <stdexcept>
using namespace prism;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); ++checks; } while (0)
int main() {
  int checks = 0;
  try {
    runtime::Registry registry;
    CHECK(std::holds_alternative<std::monostate>(registry.add("KERNEL32.DLL", "Example", 123)));
    CHECK(std::get<GuestAddress>(registry.resolve("kernel32.DlL", "Example")) == 123);
    auto missing = std::get<Diagnostic>(registry.resolve("KERNEL32.DLL", "example"));
    CHECK(missing.code == ErrorCode::unresolved_import && missing.symbol == "example" && missing.dll == "KERNEL32.DLL");
    CHECK(std::holds_alternative<Diagnostic>(registry.resolve("unknown.dll", "Example")));
    CHECK(std::holds_alternative<Diagnostic>(registry.add("kernel32.dll", "Example", 456)));
    CHECK(std::get<GuestAddress>(registry.resolve("kernel32.dll", "Example")) == 123);
    CHECK(std::holds_alternative<Diagnostic>(registry.add("", "x", 1)));
    CHECK(std::holds_alternative<Diagnostic>(registry.add("x", "", 1)));
    CHECK(std::holds_alternative<Diagnostic>(registry.add("x", "x", 0)));
    CHECK(std::holds_alternative<Diagnostic>(registry.add(std::string_view("a\0b", 3), "x", 1)));
    runtime::HandleTable table, other;
    auto resource = std::make_shared<int>(42);
    std::weak_ptr<int> lifetime = resource;
    auto h = table.insert<WindowTag>(resource);
    resource.reset();
    CHECK(*table.get<int>(h) == 42 && !lifetime.expired());
    CHECK(!table.get<int>(DcHandle{h.value}));
    CHECK(!table.get<long>(h));
    CHECK(!table.get<int>(WindowHandle{}));
    CHECK(!table.get<int>(WindowHandle{UINT64_MAX}));
    CHECK(!other.get<int>(h));
    CHECK(!table.close(DcHandle{h.value}));
    CHECK(table.close(h) && lifetime.expired());
    CHECK(!table.close(h) && !table.get<int>(h));
    auto fresh = table.insert<WindowTag>(std::make_shared<int>(7));
    CHECK(fresh != h && !table.get<int>(h) && *table.get<int>(fresh) == 7);
    auto separate = other.insert<WindowTag>(std::make_shared<int>(9));
    CHECK(separate != fresh && !other.get<int>(fresh) && !table.get<int>(separate));
    for (int i = 0; i < 10000; ++i) {
      auto old = table.insert<FileTag>(std::make_shared<int>(i));
      CHECK(table.close(old));
      CHECK(!table.get<int>(old));
    }
    std::weak_ptr<int> destruction;
    { runtime::HandleTable local; auto p = std::make_shared<int>(1); destruction = p;
      (void)local.insert<DcTag>(p); }
    CHECK(destruction.expired());
    CHECK(!runtime::current_context().thread);
    GuestProcessContext process{0x180000000}; GuestThreadContext first{1, 11}, second{2, 22};
    {
      runtime::ContextScope outer(process, first);
      CHECK(runtime::current_context().thread == &first);
      try { runtime::ContextScope inner(process, second); throw 1; } catch (int) {}
      CHECK(runtime::current_context().thread == &first);
      bool independent = false;
      std::thread worker([&] {
        independent = !runtime::current_context().thread;
        runtime::ContextScope scope(process, second);
        runtime::current_context().thread->last_error = 99;
      });
      worker.join();
      CHECK(independent && first.last_error == 11 && second.last_error == 99);
    }
    CHECK(!runtime::current_context().thread);
    std::cout << "PASS runtime checks=" << checks << '\n';
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
