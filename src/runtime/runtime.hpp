#pragma once
#include "prism/pe.hpp"
#include <atomic>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

namespace prism::runtime {
// Configure before execution; do not mutate concurrently with resolve().
class Registry final : public pe::ImportResolver {
public:
  Status add(std::string_view dll, std::string_view symbol, GuestAddress address);
  Result<GuestAddress> resolve(std::string_view dll, std::string_view symbol) override;
private:
  std::unordered_map<std::string, std::unordered_map<std::string, GuestAddress>> entries_;
};

// Worker-thread confined. Objects are native resources, never encoded pointers.
// A fresh process-wide generation prevents stale and cross-table aliases.
class HandleTable {
  struct Slot {
    std::uint32_t generation{};
    std::type_index tag{typeid(void)}, object_type{typeid(void)};
    std::shared_ptr<void> object;
  };
  std::vector<Slot> slots_;
  static std::uint32_t next_generation();
  Slot* lookup(std::uint64_t bits, std::type_index tag) noexcept;
public:
  HandleTable() = default;
  HandleTable(const HandleTable&) = delete;
  HandleTable& operator=(const HandleTable&) = delete;
  template<class Tag, class T> Handle<Tag> insert(std::shared_ptr<T> object) {
    if (!object) throw std::invalid_argument("null handle object");
    std::size_t index = 0;
    while (index < slots_.size() && slots_[index].object) ++index;
    if (index >= UINT32_MAX) throw std::overflow_error("handle slots exhausted");
    const auto generation = next_generation();
    if (index == slots_.size()) slots_.emplace_back();
    slots_[index] = {generation, typeid(Handle<Tag>), typeid(T), std::move(object)};
    return {(std::uint64_t(generation) << 32) | (index + 1)};
  }
  template<class T, class Tag> std::shared_ptr<T> get(Handle<Tag> handle) noexcept {
    auto* slot = lookup(handle.value, typeid(Handle<Tag>));
    if (!slot || slot->object_type != typeid(T)) return {};
    return std::static_pointer_cast<T>(slot->object);
  }
  template<class Tag> bool close(Handle<Tag> handle) noexcept {
    auto* slot = lookup(handle.value, typeid(Handle<Tag>));
    if (!slot) return false;
    slot->object.reset();
    return true;
  }
};

struct Binding { GuestProcessContext* process; GuestThreadContext* thread; };
// Borrowed contexts must outlive the scope. Nesting restores the prior binding.
class ContextScope {
  Binding previous_;
public:
  ContextScope(GuestProcessContext&, GuestThreadContext&) noexcept;
  ~ContextScope();
  ContextScope(const ContextScope&) = delete;
  ContextScope& operator=(const ContextScope&) = delete;
};
Binding current_context() noexcept;
}
