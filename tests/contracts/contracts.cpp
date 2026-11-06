#include "prism/abi.hpp"
#include "prism/backend.hpp"
#include "prism/pe.hpp"
#include <type_traits>
#include <utility>

static_assert(prism::contract_version == 1);
static_assert(!std::is_convertible_v<prism::WindowHandle, prism::DcHandle>);
static_assert(!std::is_copy_constructible_v<prism::pe::MappedImage>);
static_assert(!std::is_copy_constructible_v<prism::pe::Inspection>);
static_assert(std::is_standard_layout_v<prism::win::WNDCLASSEXW>);
#if PRISM_HAS_X64_GUEST_ABI
static_assert(!std::is_same_v<prism::win::EntryPoint, std::uint32_t (*)() noexcept>);
#endif
namespace {
struct Mapping final : prism::pe::MappedImage {
  bool& destroyed;
  explicit Mapping(bool& value) : destroyed(value) {}
  ~Mapping() override { destroyed = true; }
  prism::GuestAddress base() const noexcept override { return 0x10000; }
  std::uint64_t size() const noexcept override { return 0x2000; }
  prism::GuestAddress entry_address() const noexcept override { return 0x11000; }
};
}
int main() {
  bool destroyed = false;
  {
    prism::Result<std::unique_ptr<prism::pe::MappedImage>> result = std::make_unique<Mapping>(destroyed);
    auto owner = std::move(std::get<0>(result));
    if (std::get<0>(result) || owner->entry_address() != 0x11000 || destroyed) return 1;
  }
  if (!destroyed) return 2;
  prism::Diagnostic diagnostic;
  diagnostic.code = prism::ErrorCode::unresolved_import;
  diagnostic.dll = "KERNEL32.DLL";
  diagnostic.symbol = "Missing";
  prism::Result<prism::GuestAddress> failure = diagnostic;
  if (!std::holds_alternative<prism::Diagnostic>(failure)) return 3;
  return 0;
}
