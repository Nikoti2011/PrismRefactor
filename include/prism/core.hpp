#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace prism {
inline constexpr std::uint32_t contract_version = 1;
using GuestAddress = std::uint64_t;
using Rva = std::uint32_t;
using WindowsError = std::uint32_t;
enum class ErrorCode { malformed_image, unsupported_feature, unresolved_import,
                       invalid_handle, invalid_argument, native_failure, execution_unavailable };
struct Diagnostic {
  ErrorCode code{};
  std::string event, guest_module;
  std::optional<Rva> guest_rva;
  std::string dll, symbol;
  std::optional<std::uint64_t> guest_thread;
  std::optional<std::int64_t> result;
  std::optional<WindowsError> windows_error;
  std::optional<int> native_error;
  std::string unsupported_reason;
};
template<class T> using Result = std::variant<T, Diagnostic>;
using Status = Result<std::monostate>;
template<class Tag> struct Handle {
  std::uint64_t value{};
  explicit operator bool() const noexcept { return value != 0; }
  bool operator==(const Handle&) const = default;
};
using WindowHandle = Handle<struct WindowTag>;
using DcHandle = Handle<struct DcTag>;
using FileHandle = Handle<struct FileTag>;
// Host context, never passed as a Windows ABI structure.
struct GuestThreadContext {
  std::uint64_t id{};
  WindowsError last_error{};
};
struct GuestProcessContext { GuestAddress image_base{}; };
static_assert(sizeof(GuestAddress) == 8 && sizeof(WindowsError) == 4);
}
