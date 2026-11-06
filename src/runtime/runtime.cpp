#include "runtime.hpp"
#include <limits>
#include <stdexcept>

namespace prism::runtime {
namespace {
thread_local Binding binding{};
std::atomic<std::uint64_t> generation_counter{1};
std::string normalized(std::string_view name) {
  std::string result(name);
  for (auto& c : result) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return result;
}
Diagnostic error(ErrorCode code, std::string_view dll, std::string_view symbol,
                 const char* reason) {
  Diagnostic d; d.code = code; d.event = "import_resolution";
  d.dll = dll; d.symbol = symbol; d.unsupported_reason = reason; return d;
}
bool valid(std::string_view s) { return !s.empty() && s.find('\0') == s.npos; }
}
Status Registry::add(std::string_view dll, std::string_view symbol, GuestAddress address) {
  if (!valid(dll) || !valid(symbol) || !address)
    return error(ErrorCode::invalid_argument, dll, symbol, "nonempty names and nonzero ABI address required");
  auto& symbols = entries_[normalized(dll)];
  if (!symbols.emplace(std::string(symbol), address).second)
    return error(ErrorCode::invalid_argument, dll, symbol, "duplicate import registration");
  return std::monostate{};
}
Result<GuestAddress> Registry::resolve(std::string_view dll, std::string_view symbol) {
  const auto library = entries_.find(normalized(dll));
  if (library != entries_.end()) {
    const auto entry = library->second.find(std::string(symbol));
    if (entry != library->second.end()) return entry->second;
  }
  return error(ErrorCode::unresolved_import, dll, symbol, "DLL/symbol is not registered");
}
std::uint32_t HandleTable::next_generation() {
  auto value = generation_counter.load(std::memory_order_relaxed);
  for (;;) {
    if (value > UINT32_MAX) throw std::overflow_error("handle generations exhausted");
    if (generation_counter.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
      return static_cast<std::uint32_t>(value);
  }
}
HandleTable::Slot* HandleTable::lookup(std::uint64_t bits, std::type_index tag) noexcept {
  const auto index = static_cast<std::uint32_t>(bits);
  if (!index || index > slots_.size()) return nullptr;
  auto& slot = slots_[index - 1];
  if (!slot.object || slot.generation != (bits >> 32) || slot.tag != tag) return nullptr;
  return &slot;
}
ContextScope::ContextScope(GuestProcessContext& process, GuestThreadContext& thread) noexcept
  : previous_(binding) { binding = {&process, &thread}; }
ContextScope::~ContextScope() { binding = previous_; }
Binding current_context() noexcept { return binding; }
}
