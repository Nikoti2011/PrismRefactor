#include "bridge.hpp"
#include "bootstrap_bytes.hpp"
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
unsigned calls;
prism::win::WindowProc callback;
extern "C" __attribute__((noinline)) std::uint64_t native_sum(
    std::uint64_t a, std::uint64_t b, std::uint64_t c,
    std::uint64_t d, std::uint64_t e, std::uint64_t f) noexcept {
  return a + 2*b + 3*c + 4*d + 5*e + 6*f;
}
std::uint64_t PRISM_GUEST_ABI adapter(std::uint64_t a, std::uint64_t b,
    std::uint64_t c, std::uint64_t d, std::uint64_t e, std::uint64_t f) noexcept {
  ++calls;
  auto back = prism::abi::invoke_window_proc(callback, 10, 20, 30, -4);
  auto result = native_sum(a,b,c,d,e,f);
  std::printf("guest_import PrismProbe6 args=%llu,%llu,%llu,%llu,%llu,%llu callback=%lld result=%llu\n",
    a,b,c,d,e,f,back,result);
  return back == 56 ? result : 0;
}
}
int main(int argc, char** argv) {
  if (argc != 2) { std::fprintf(stderr,"usage: prism_execution_probe bootstrap.exe\n"); return 2; }
  std::ifstream stream(argv[1], std::ios::binary);
  std::vector<unsigned char> file((std::istreambuf_iterator<char>(stream)), {});
  // This experiment accepts exactly our bootstrap, not arbitrary PE input.
  // General PE validation/mapping belongs to P03/P05.
  if(file.size()!=sizeof bootstrap_bytes || std::memcmp(file.data(),bootstrap_bytes,file.size())) {
    std::fprintf(stderr,"rejected: input differs from pinned bootstrap; no mapping/entry\n"); return 3;
  }
  int translated = 0; size_t size = sizeof translated;
  if(sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) != 0) translated = -1;
  auto page = sysconf(_SC_PAGESIZE);
  std::printf("host=x86_64 translated=%d page_size=%ld\n",translated,page);
  if(page <= 0 || 0x4000 % page) return 4;
  auto* base = static_cast<unsigned char*>(mmap(nullptr,0xc000,PROT_READ|PROT_WRITE,
                                               MAP_PRIVATE|MAP_ANON,-1,0));
  if(base == MAP_FAILED) { std::perror("mmap RW"); return 5; }
  std::memcpy(base,file.data(),0x200);
  std::memcpy(base+0x4000,file.data()+0x200,0x200);
  std::memcpy(base+0x8000,file.data()+0x400,0x200);
  const auto address = reinterpret_cast<std::uintptr_t>(&adapter);
  std::memcpy(base+0x8060,&address,sizeof address);
  // No RWX interval, MAP_JIT, MAP_FIXED, entitlements or security changes.
  if(mprotect(base,0x4000,PROT_READ) ||
     mprotect(base+0x4000,0x4000,PROT_READ|PROT_EXEC) ||
     mprotect(base+0x8000,0x4000,PROT_READ)) {
    std::perror("mprotect final"); munmap(base,0xc000); return 6;
  }
  callback = reinterpret_cast<prism::win::WindowProc>(base+0x4100);
  std::printf("mapped=%p preferred=0x180000000 protections=R,RX,R entry_rva=0x4000\n",base);
  std::fflush(stdout);
  auto result = prism::abi::invoke_entry(reinterpret_cast<prism::win::EntryPoint>(base+0x4000));
  callback = nullptr;
  bool unmapped = munmap(base,0xc000) == 0;
  std::printf("guest_return=0x%x import_calls=%u unmapped=%d\n",result,calls,unmapped);
  return result == 0x50415353 && calls == 1 && unmapped ? 0 : 7;
}
