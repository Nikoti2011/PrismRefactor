#include "bridge.hpp"
#include <cstdio>
#include <cstdint>
using U = std::uint64_t;
using Mixed = double (PRISM_GUEST_ABI *)(U,double,U,double,double,U) noexcept;
using Void = void (PRISM_GUEST_ABI *)() noexcept;
extern "C" U PRISM_GUEST_ABI alignment() noexcept;
extern "C" int preserved(Void) noexcept;
extern "C" void native_clobber() noexcept;
extern "C" U native_alignment() noexcept;
__attribute__((noinline)) U PRISM_GUEST_ABI check_native_alignment() noexcept { return native_alignment(); }
extern "C" void PRISM_GUEST_ABI bad_guest() noexcept;
extern "C" __attribute__((noinline)) double native_mixed(U a,double b,U c,double d,double e,U f) noexcept {
  return a==0x123456789abcdef0ULL && c==0xfedcba9876543210ULL && f==0xaabbccddeeff0011ULL
    ? b+2*d+3*e : -1;
}
__attribute__((noinline)) double PRISM_GUEST_ABI mixed(U a,double b,U c,double d,double e,U f) noexcept {
  return native_mixed(a,b,c,d,e,f);
}
__attribute__((noinline)) double native_callback(Mixed callback) noexcept {
  return callback(0x123456789abcdef0ULL,1.25,0xfedcba9876543210ULL,2.5,4.25,0xaabbccddeeff0011ULL);
}
__attribute__((noinline)) double PRISM_GUEST_ABI round_trip(Mixed callback) noexcept {
  return native_callback(callback);
}
void PRISM_GUEST_ABI adapter_clobber() noexcept { native_clobber(); }
prism::win::LRESULT PRISM_GUEST_ABI wnd(prism::win::HWND h,prism::win::DWORD m,
    prism::win::WPARAM w,prism::win::LPARAM l) noexcept {
  return h==0x1122334455667788ULL && m==0xaabbccdd && w==0xfedcba9876543210ULL && l==-123
    ? -0x123456789LL : 0;
}
std::uint32_t PRISM_GUEST_ABI entry() noexcept { return 0xfedcba98; }
int main() {
  if(prism::abi::invoke_entry(entry)!=0xfedcba98) return 1;
  if(prism::abi::invoke_window_proc(wnd,0x1122334455667788ULL,0xaabbccdd,0xfedcba9876543210ULL,-123)
     !=-0x123456789LL) return 2;
  if(native_callback(mixed)!=19.0 || round_trip(mixed)!=19.0) return 3;
  if(alignment()!=8 || check_native_alignment()!=8) return 4;
  if(preserved(adapter_clobber)!=0) return 5;
  if(preserved(bad_guest)==0) return 6; // prove preservation harness detects a violation
  std::puts("PASS entry/window signed and wide returns; mixed integer/FP register+stack arguments both directions; shadow writes/alignment; RBX/RBP/RDI/RSI/R12-R15/XMM6-XMM15 preserved; corrupting control rejected");
}
