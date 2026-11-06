#pragma once
#include "prism/core.hpp"
#include <memory>
#include <span>
#include <string>

namespace prism::backend {
using WindowId = Handle<struct NativeWindowTag>;
enum class Key { up, down, left, right, escape, space, enter, r, unknown };
enum class EventKind { key_down, key_up, focus_lost, focus_gained, close_requested };
struct Event { WindowId window; EventKind kind{}; Key key = Key::unknown; bool repeat{}; };
struct WindowSpec { std::u16string title; std::uint32_t width{}, height{}; };
struct ClientSize { std::uint32_t width{}, height{}; };
// Top-down B,G,R,X bytes, X ignored; stride >= width*4, checked for overflow.
// Backend copies before return; one logical pixel per source pixel, no crop.
struct Pixels {
  std::span<const std::byte> bytes;
  std::uint32_t width{}, height{};
  std::size_t stride{};
};
// All methods run on main thread. P07 marshals calls, not guest callbacks.
// poll_event is nonblocking. UI event producers must never wait for guest work.
class Display {
public:
  virtual ~Display() = default;
  virtual Result<WindowId> create(const WindowSpec&) = 0;
  virtual Status show(WindowId) = 0;
  virtual Result<ClientSize> client_size(WindowId) = 0;
  virtual Status present(WindowId, const Pixels&) = 0;
  virtual Status destroy(WindowId) = 0;
  virtual std::optional<Event> poll_event() = 0;
};
// Construct and destroy on main thread. Factory supplied by P04.
Result<std::unique_ptr<Display>> make_display();
}
