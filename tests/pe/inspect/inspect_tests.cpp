#include "prism/pe.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
using Bytes = std::vector<std::byte>;
using prism::ErrorCode;
namespace {
unsigned checks = 0;
void check(bool value, const std::string& message) {
  ++checks;
  if (!value) throw std::runtime_error(message);
}
void put(Bytes& b, std::size_t at, std::uint64_t n, unsigned width = 4) {
  for (unsigned i = 0; i < width; ++i) b.at(at + i) = std::byte((n >> (8 * i)) & 255);
}
void text(Bytes& b, std::size_t at, const std::string& s) {
  for (char c : s) b.at(at++) = std::byte(c);
  b.at(at) = std::byte{0};
}
constexpr unsigned opt = 0x98, table = opt + 240;
void directory(Bytes& b, unsigned index, unsigned rva, unsigned size) {
  put(b, opt + 112 + index * 8, rva); put(b, opt + 116 + index * 8, size);
}
// P03-only structural input, not a runnable/common guest fixture.
Bytes sample() {
  Bytes b(0x800);
  put(b, 0, 0x5a4d, 2); put(b, 0x3c, 0x80); put(b, 0x80, 0x4550);
  put(b, 0x84, 0x8664, 2); put(b, 0x86, 3, 2);
  put(b, 0x94, 240, 2); put(b, 0x96, 0x22, 2);
  put(b, opt, 0x20b, 2); put(b, opt + 16, 0x1000);
  put(b, opt + 24, 0x180000000, 8); put(b, opt + 32, 0x1000);
  put(b, opt + 36, 0x200); put(b, opt + 56, 0x4000); put(b, opt + 60, 0x200);
  put(b, opt + 68, 3, 2); put(b, opt + 108, 16);
  for (unsigned i = 0; i < 3; ++i) {
    auto p = table + i * 40;
    text(b, p, i == 0 ? ".text" : i == 1 ? ".idata" : ".reloc");
    put(b, p + 8, i == 1 ? 0x300 : 0x200); put(b, p + 12, 0x1000 * (i + 1));
    put(b, p + 16, 0x200); put(b, p + 20, 0x200 * (i + 1));
    put(b, p + 36, i == 0 ? 0x60000020 : i == 1 ? 0xc0000040 : 0x42000040);
  }
  directory(b, 1, 0x2000, 40); directory(b, 5, 0x3000, 12); directory(b, 12, 0x2060, 16);
  put(b, 0x400, 0x2040); put(b, 0x40c, 0x2080); put(b, 0x410, 0x2060);
  put(b, 0x440, 0x20a0, 8); put(b, 0x460, 0x20a0, 8);
  text(b, 0x480, "KeRnEl32.DLL"); text(b, 0x4a2, "OutputDebugStringA");
  put(b, 0x4c0, 0x180001000, 8);
  put(b, 0x600, 0x2000); put(b, 0x604, 12); put(b, 0x608, 0xa0c0, 2);
  return b;
}
void reject(const Bytes& b, ErrorCode code, const std::string& label) {
  auto result = prism::pe::inspect(b);
  const auto* d = std::get_if<prism::Diagnostic>(&result);
  check(d && d->code == code && d->event == "pe.inspect" && !d->unsupported_reason.empty(), label);
}
void accept(const Bytes& b, const std::string& label) {
  auto result = prism::pe::inspect(b);
  if (auto* d = std::get_if<prism::Diagnostic>(&result))
    throw std::runtime_error(label + ": " + d->unsupported_reason);
  ++checks;
}
void suite() {
  auto b = sample();
  auto result = prism::pe::inspect(b);
  check(std::holds_alternative<prism::pe::Inspection>(result), "valid image accepted");
  auto& image = *std::get<prism::pe::Inspection>(result);
  const auto& m = image.info();
  check(m.machine == 0x8664 && m.preferred_base == 0x180000000 && m.entry_rva == 0x1000,
        "machine/base/entry metadata");
  check(m.image_size == 0x4000 && m.headers_size == 0x200 && m.sections.size() == 3,
        "image dimensions");
  check(m.sections[1].virtual_size == 0x300 && m.sections[1].raw_size == 0x200,
        "virtual zero tail preserved");
  check(m.imports.size() == 1 && m.imports[0].dll == "kernel32.dll" &&
        m.imports[0].symbol == "OutputDebugStringA" && m.imports[0].iat_rva == 0x2060,
        "normalized DLL and case-preserved symbol/IAT");
  check(m.relocations.size() == 2 && m.relocations[0].type == 10 &&
        m.relocations[0].rva == 0x20c0 && m.relocations[1].type == 0, "DIR64 and ABSOLUTE metadata");
  b[0] = std::byte{0}; b.clear(); b.shrink_to_fit();
  check(image.bytes().size() == 0x800 && image.bytes()[0] == std::byte{0x4d}, "owned immutable snapshot");
  auto mutate = [](std::size_t at, std::uint64_t value, unsigned width,
                   ErrorCode code, const char* label) {
    auto bytes = sample(); put(bytes, at, value, width); reject(bytes, code, label);
  };
  const auto malformed = ErrorCode::malformed_image, unsupported = ErrorCode::unsupported_feature;
  mutate(0, 0, 2, malformed, "DOS signature");
  mutate(0x3c, 0xfffffff0, 4, malformed, "overflowing PE offset");
  mutate(0x80, 0, 4, malformed, "PE signature");
  mutate(0x84, 0xaa64, 2, unsupported, "ARM image");
  mutate(0x86, 97, 2, malformed, "section count limit");
  mutate(0x94, 112, 2, malformed, "directory array truncation");
  mutate(0x96, 0x2022, 2, unsupported, "DLL image");
  mutate(opt, 0x10b, 2, unsupported, "PE32");
  mutate(opt + 16, 0x2000, 4, malformed, "nonexecuting entry");
  mutate(opt + 16, 0x1200, 4, malformed, "entry in gap");
  b = sample(); put(b, opt + 24, 0xffffffffffff0000ULL, 8); put(b, opt + 56, 0x20000);
  reject(b, malformed, "base arithmetic overflow");
  mutate(opt + 32, 0x1800, 4, malformed, "non-power-of-two alignment");
  mutate(opt + 32, 0x200, 4, unsupported, "low section alignment");
  mutate(opt + 36, 0, 4, malformed, "zero file alignment");
  mutate(opt + 56, 0xfffff000, 4, unsupported, "image allocation limit");
  mutate(opt + 60, 0x100, 4, malformed, "header alignment");
  mutate(opt + 68, 1, 2, unsupported, "native subsystem");
  mutate(opt + 70, 0x4000, 2, unsupported, "CFG");
  mutate(table + 12, 0xfffff000, 4, malformed, "section RVA overflow");
  mutate(table + 16, 0xfffffe00, 4, malformed, "raw size overflow");
  mutate(table + 20, 0xfffffe00, 4, malformed, "raw offset overflow");
  mutate(table + 36, 0xe0000020, 4, unsupported, "RWX section");
  mutate(table + 40 + 12, 0x1000, 4, malformed, "overlapping sections");
  mutate(table + 40 + 20, 0x200, 4, malformed, "overlapping raw ranges");
  mutate(0x40c, 0x2200, 4, malformed, "DLL name in zero tail");
  mutate(0x40c, 0x1800, 4, malformed, "DLL name in virtual gap");
  mutate(0x410, 0xfffffffc, 4, malformed, "bad IAT");
  mutate(0x440, 0x1000020a0ULL, 8, malformed, "64-bit import name RVA");
  mutate(0x460, 0x20b0, 8, malformed, "IAT mismatch");
  mutate(0x604, 0, 4, malformed, "zero relocation block");
  mutate(0x604, 10, 4, malformed, "unaligned relocation block");
  mutate(0x604, 16, 4, malformed, "overlong relocation block");
  mutate(0x600, 0xfffff000, 4, malformed, "relocation page outside image");
  mutate(0x608, 0x30c0, 2, unsupported, "unsupported HIGHLOW relocation");
  mutate(0x608, 0xafff, 2, malformed, "DIR64 crossing section end");
  mutate(0x60a, 0xa0c4, 2, malformed, "overlapping DIR64 fixups");
  mutate(0x96, 0x23, 2, malformed, "stripped relocations contradiction");
  for (unsigned index : {0, 2, 4, 7, 8, 9, 10, 11, 13, 14, 15}) {
    b = sample(); directory(b, index, index == 4 ? 0x600 : 0x3000, 12);
    reject(b, unsupported, "unsupported directory " + std::to_string(index));
  }
  b = sample(); directory(b, 1, 0x2000, 20); reject(b, malformed, "no import descriptor terminator");
  b = sample(); directory(b, 1, 0x2000, 19); reject(b, malformed, "partial import descriptor");
  b = sample(); directory(b, 5, 0x3000, 7); reject(b, malformed, "short relocation header");
  b = sample(); directory(b, 12, 0x2200, 8); reject(b, malformed, "directory in zero tail");
  b = sample(); directory(b, 3, 0x30c0, 11); reject(b, malformed, "partial exception entry");
  b = sample(); directory(b, 3, 0x30c0, 12); accept(b, "opaque exception directory");
  auto e = prism::pe::inspect(b);
  check(std::get<prism::pe::Inspection>(e)->info().exception_directory.rva == 0x30c0,
        "exception metadata retained");
  b = sample(); put(b, 0x440, 0x8000000000000001ULL, 8); put(b, 0x460, 0x8000000000000001ULL, 8);
  reject(b, unsupported, "ordinal import");
  auto ordinal = prism::pe::inspect(b);
  check(std::get<prism::Diagnostic>(ordinal).dll == "kernel32.dll", "ordinal diagnostic DLL context");
  b = sample(); put(b, 0x400, 0); accept(b, "OriginalFirstThunk zero falls back to IAT");
  b = sample(); directory(b, 1, 0, 0); directory(b, 12, 0, 0); accept(b, "no imports");
  b = sample(); directory(b, 5, 0, 0); put(b, 0x96, 0x23, 2); accept(b, "nonrelocatable image metadata");
  b = sample(); put(b, 0x608, 0xa280, 2); accept(b, "DIR64 in zero-filled tail");
  b = sample(); put(b, table + 8, 0); accept(b, "zero VirtualSize uses raw extent");
  b = sample(); put(b, 0x40c, 0x21ff); b[0x5ff] = std::byte{'A'};
  reject(b, malformed, "unterminated string at raw end");
  b = sample(); put(b, 0x400, 0x21f8); put(b, 0x410, 0x21f8); put(b, 0x5f8, 0x20a0, 8);
  reject(b, malformed, "unterminated thunk table");
  const auto original = sample();
  for (std::size_t size = 0; size < original.size(); ++size) {
    Bytes truncated(original.begin(), original.begin() + size);
    reject(truncated, malformed, "truncation at " + std::to_string(size));
  }
  // Reproducible mutation campaign: every accepted result must retain safe ranges.
  std::mt19937 rng(0x503033);
  for (unsigned trial = 0; trial < 20000; ++trial) {
    b = original;
    for (unsigned j = 0, n = 1 + rng() % 8; j < n; ++j) b[rng() % b.size()] = std::byte(rng() & 255);
    auto r = prism::pe::inspect(b);
    if (const auto* valid = std::get_if<prism::pe::Inspection>(&r)) {
      check((*valid)->bytes().size() == b.size(), "mutation snapshot size");
      for (const auto& s : (*valid)->info().sections)
        check(!s.raw_size || std::uint64_t(s.file_offset) + s.raw_size <= b.size(), "accepted raw bounds");
    } else {
      check(!std::get<prism::Diagnostic>(r).unsupported_reason.empty(), "mutation diagnosis");
    }
  }
}
}
int main(int argc, char** argv) {
  try {
    if (argc == 2) {
      std::ifstream input(argv[1], std::ios::binary);
      if (!input) throw std::runtime_error("cannot open input");
      std::vector<char> chars((std::istreambuf_iterator<char>(input)), {});
      Bytes bytes; for (auto c : chars) bytes.push_back(std::byte(c));
      auto result = prism::pe::inspect(bytes);
      if (auto* d = std::get_if<prism::Diagnostic>(&result)) {
        std::cerr << "REJECT code=" << static_cast<int>(d->code) << " " << d->unsupported_reason << '\n'; return 2;
      }
      const auto& m = std::get<prism::pe::Inspection>(result)->info();
      std::cout << "ACCEPT entry=" << m.entry_rva << " sections=" << m.sections.size()
                << " imports=" << m.imports.size() << " relocations=" << m.relocations.size() << '\n';
      for (const auto& i : m.imports) std::cout << i.dll << '!' << i.symbol << " iat=" << i.iat_rva << '\n';
      return 0;
    }
    suite();
    std::cout << "PASS " << checks << " checks\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n'; return 1;
  }
}
