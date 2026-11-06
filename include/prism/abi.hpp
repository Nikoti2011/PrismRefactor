#pragma once
#include "prism/core.hpp"

// Never silently turn an x64 Windows call into an ARM64 native call.
#if defined(__x86_64__) && (defined(__clang__) || defined(__GNUC__))
#define PRISM_HAS_X64_GUEST_ABI 1
#define PRISM_GUEST_ABI __attribute__((ms_abi))
#else
#define PRISM_HAS_X64_GUEST_ABI 0
#endif
namespace prism::win {
using BOOL = std::int32_t;
using LONG = std::int32_t;
using DWORD = std::uint32_t;
using WORD = std::uint16_t;
using WCHAR = char16_t;
using WPARAM = std::uint64_t;
using LPARAM = std::int64_t;
using LRESULT = std::int64_t;
// Guest-visible handle bits; P06 validates kind, generation, and lifetime.
using HANDLE = std::uint64_t;
using HWND = HANDLE;
struct POINT { LONG x, y; };
struct RECT { LONG left, top, right, bottom; };
struct LARGE_INTEGER { std::int64_t QuadPart; };
struct MSG {
  HWND hwnd; DWORD message; WPARAM wParam; LPARAM lParam;
  DWORD time; POINT pt; DWORD lPrivate;
};
// Pointer fields are guest addresses, not native wchar_t*/long/objects.
struct WNDCLASSEXW {
  DWORD cbSize, style; GuestAddress lpfnWndProc;
  std::int32_t cbClsExtra, cbWndExtra;
  HANDLE hInstance, hIcon, hCursor, hbrBackground;
  GuestAddress lpszMenuName, lpszClassName; HANDLE hIconSm;
};
struct CREATESTRUCTW {
  GuestAddress lpCreateParams; HANDLE hInstance, hMenu; HWND hwndParent;
  std::int32_t cy, cx, y, x; LONG style;
  GuestAddress lpszName, lpszClass; DWORD dwExStyle;
};
struct BITMAPINFOHEADER {
  DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount;
  DWORD biCompression, biSizeImage; LONG biXPelsPerMeter, biYPelsPerMeter;
  DWORD biClrUsed, biClrImportant;
};
static_assert(sizeof(RECT) == 16 && alignof(RECT) == 4);
static_assert(sizeof(LARGE_INTEGER) == 8 && alignof(LARGE_INTEGER) == 8);
static_assert(sizeof(MSG) == 48 && alignof(MSG) == 8 && offsetof(MSG, wParam) == 16 && offsetof(MSG, pt) == 36);
static_assert(sizeof(WNDCLASSEXW) == 80 && alignof(WNDCLASSEXW) == 8 && offsetof(WNDCLASSEXW, lpszClassName) == 64);
static_assert(sizeof(CREATESTRUCTW) == 80 && alignof(CREATESTRUCTW) == 8 && offsetof(CREATESTRUCTW, lpszName) == 56);
static_assert(sizeof(BITMAPINFOHEADER) == 40 && alignof(BITMAPINFOHEADER) == 4 && offsetof(BITMAPINFOHEADER, biCompression) == 16);
static_assert(sizeof(WCHAR) == 2 && sizeof(LONG) == 4 && sizeof(HWND) == 8);
#if PRISM_HAS_X64_GUEST_ABI
using EntryPoint = std::uint32_t (PRISM_GUEST_ABI *)() noexcept;
using WindowProc = LRESULT (PRISM_GUEST_ABI *)(HWND, DWORD, WPARAM, LPARAM) noexcept;
using DebugOutput = void (PRISM_GUEST_ABI *)(const char*) noexcept;
#endif
// Cross-architecture inspection stores callback addresses as GuestAddress.
// Availability of these types does NOT establish working ABI bridges (P01).
}
