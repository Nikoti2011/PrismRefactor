#include "prism/pe.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <set>

namespace prism::pe {
namespace {
// Deliberate resource limits for the small-fixture contract, not PE format limits.
constexpr std::size_t max_file = 64 * 1024 * 1024;
constexpr std::uint64_t max_image = 256 * 1024 * 1024;
constexpr std::size_t max_records = 65536;
constexpr std::uint32_t execute = 0x20000000, write = 0x80000000;
[[noreturn]] void fail(std::string reason, ErrorCode code = ErrorCode::malformed_image,
                       std::optional<Rva> rva = {}) {
  Diagnostic d;
  d.code = code;
  d.event = "pe.inspect";
  d.guest_rva = rva;
  d.unsupported_reason = std::move(reason);
  throw d;
}
void require(bool condition, const char* reason) { if (!condition) fail(reason); }
void supported(bool condition, const char* reason) {
  if (!condition) fail(reason, ErrorCode::unsupported_feature);
}
bool power2(std::uint32_t n) { return n && !(n & (n - 1)); }
bool range(std::uint64_t start, std::uint64_t size, std::uint64_t limit) {
  return start <= limit && size <= limit - start;
}
bool overlap(std::uint64_t a, std::uint64_t n, std::uint64_t b, std::uint64_t m) {
  return n && m && a < b + m && b < a + n;
}
class Snapshot final : public InspectedImage {
public:
  std::vector<std::byte> data;
  ImageInfo metadata;
  const ImageInfo& info() const noexcept override { return metadata; }
  std::span<const std::byte> bytes() const noexcept override { return data; }
};
class Parser {
  ImageInfo& info;
  std::span<const std::byte> data;
  std::array<Directory, 16> directories{};
  std::set<Rva> iat_slots;
  std::set<Rva> fixups;
  std::uint64_t u(std::uint64_t pos, unsigned width) const {
    require(range(pos, width, data.size()), "truncated file field");
    std::uint64_t value = 0;
    for (unsigned i = 0; i < width; ++i)
      value |= std::uint64_t(std::to_integer<unsigned char>(data[pos + i])) << (8 * i);
    return value;
  }
  // Map only bytes present in the file, never virtual zero tails or gaps.
  std::uint64_t offset(std::uint64_t rva, std::uint64_t size) const {
    require(range(rva, size, info.image_size), "RVA outside image");
    if (range(rva, size, info.headers_size)) return rva;
    for (const auto& s : info.sections)
      if (rva >= s.rva && range(rva - s.rva, size, s.raw_size))
        return std::uint64_t(s.file_offset) + rva - s.rva;
    fail("RVA range is not contiguous file-backed data", ErrorCode::malformed_image,
         static_cast<Rva>(rva));
  }
  const Section* section(std::uint64_t rva, std::uint64_t size) const {
    for (const auto& s : info.sections)
      if (rva >= s.rva && range(rva - s.rva, size, std::max(s.virtual_size, s.raw_size)))
        return &s;
    return nullptr;
  }
  std::string name(std::uint64_t rva) const {
    std::string value;
    for (unsigned i = 0; i <= 4096; ++i) {
      auto c = u(offset(rva + i, 1), 1);
      if (!c) { require(!value.empty(), "empty import name"); return value; }
      supported(i < 4096, "import name exceeds 4096 bytes");
      require(c >= 32 && c <= 126, "non-ASCII import name");
      value.push_back(static_cast<char>(c));
    }
    fail("unterminated import name");
  }
  void imports() {
    const auto d = directories[1];
    if (!d.size) return;
    bool terminated = false;
    for (std::uint64_t at = 0; range(at, 20, d.size); at += 20) {
      auto p = offset(std::uint64_t(d.rva) + at, 20);
      auto lookup = u(p, 4), stamp = u(p + 4, 4), chain = u(p + 8, 4);
      auto dll_rva = u(p + 12, 4), iat = u(p + 16, 4);
      if (!(lookup | stamp | chain | dll_rva | iat)) { terminated = true; break; }
      supported(at / 20 < 1024, "too many import descriptors");
      supported(stamp == 0 && chain == 0, "bound/forwarder import descriptor unsupported");
      require(dll_rva && iat, "missing DLL name or IAT");
      auto dll = name(dll_rva);
      for (auto& c : dll) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
      if (!lookup) lookup = iat;
      require(lookup % 8 == 0 && iat % 8 == 0, "unaligned import thunk table");
      for (std::uint64_t index = 0;; ++index) {
        supported(index < max_records, "too many import thunks");
        auto slot = iat + index * 8;
        auto thunk = u(offset(lookup + index * 8, 8), 8);
        auto initial_iat = u(offset(slot, 8), 8);
        require(section(slot, 8) != nullptr, "IAT slot outside a section");
        require(initial_iat == thunk, "bound or inconsistent IAT");
        if (!thunk) break;
        if (thunk >> 63) {
          Diagnostic e; e.code = ErrorCode::unsupported_feature; e.event = "pe.inspect";
          e.dll = dll; e.guest_rva = static_cast<Rva>(slot);
          e.unsupported_reason = "ordinal import unsupported"; throw e;
        }
        require(thunk <= std::numeric_limits<Rva>::max(), "import name RVA overflow");
        (void)offset(thunk, 2); // IMAGE_IMPORT_BY_NAME hint
        auto symbol = name(thunk + 2);
        supported(info.imports.size() < max_records, "too many imports");
        require(iat_slots.insert(static_cast<Rva>(slot)).second, "overlapping IAT slots");
        info.imports.push_back({dll, std::move(symbol), static_cast<Rva>(slot)});
      }
    }
    require(terminated, "unterminated import descriptor table");
  }
  void relocations() {
    auto d = directories[5];
    require(!info.relocations_stripped || !d.size, "relocations present but marked stripped");
    for (std::uint64_t at = 0; at < d.size;) {
      require(range(at, 8, d.size), "truncated relocation block header");
      auto p = offset(std::uint64_t(d.rva) + at, 8);
      auto page = u(p, 4), size = u(p + 4, 4);
      require(page % 4096 == 0 && page < info.image_size, "invalid relocation page");
      require(size >= 8 && size % 4 == 0 && range(at, size, d.size), "invalid relocation block size");
      for (std::uint64_t j = 8; j < size; j += 2) {
        auto record = u(offset(std::uint64_t(d.rva) + at + j, 2), 2);
        auto type = static_cast<std::uint16_t>(record >> 12);
        auto target = page + (record & 4095);
        supported(type == 0 || type == 10, "unsupported relocation type (only ABSOLUTE/DIR64)");
        if (type == 10) {
          require(section(target, 8) != nullptr, "DIR64 target outside a section");
          auto next = fixups.lower_bound(static_cast<Rva>(target));
          require(next == fixups.end() || target + 8 <= *next, "overlapping DIR64 targets");
          require(next == fixups.begin() || std::uint64_t(*std::prev(next)) + 8 <= target,
                  "overlapping DIR64 targets");
          fixups.insert(static_cast<Rva>(target));
        }
        supported(info.relocations.size() < max_records, "too many relocation records");
        info.relocations.push_back({static_cast<Rva>(target), type});
      }
      at += size;
    }
  }
public:
  explicit Parser(Snapshot& s) : info(s.metadata), data(s.data) {}
  void parse() {
    require(u(0, 2) == 0x5a4d, "missing DOS signature");
    auto pe = u(0x3c, 4);
    require(pe >= 64 && u(pe, 4) == 0x4550, "missing PE signature");
    auto coff = pe + 4;
    info.machine = static_cast<std::uint16_t>(u(coff, 2));
    supported(info.machine == 0x8664, "only AMD64 images supported");
    auto count = u(coff + 2, 2), optional_size = u(coff + 16, 2), flags = u(coff + 18, 2);
    require(count > 0 && count <= 96, "invalid section count");
    require(flags & 2, "not an executable image");
    supported(!(flags & 0x2000), "guest DLL images unsupported");
    info.relocations_stripped = flags & 1;
    auto opt = coff + 20;
    require(optional_size >= 112 && range(opt, optional_size, data.size()), "truncated optional header");
    supported(u(opt, 2) == 0x20b, "only PE32+ images supported");
    info.entry_rva = static_cast<Rva>(u(opt + 16, 4));
    info.preferred_base = u(opt + 24, 8);
    info.section_alignment = static_cast<std::uint32_t>(u(opt + 32, 4));
    info.file_alignment = static_cast<std::uint32_t>(u(opt + 36, 4));
    info.image_size = static_cast<std::uint32_t>(u(opt + 56, 4));
    info.headers_size = static_cast<std::uint32_t>(u(opt + 60, 4));
    require(power2(info.file_alignment) && info.file_alignment >= 512 && info.file_alignment <= 65536,
            "invalid file alignment");
    require(power2(info.section_alignment) && info.section_alignment >= info.file_alignment,
            "invalid section alignment");
    supported(info.section_alignment >= 4096, "low section alignment unsupported");
    require(info.image_size && info.image_size % info.section_alignment == 0, "invalid image size");
    supported(info.image_size <= max_image, "image exceeds 256 MiB policy limit");
    require(info.preferred_base % 65536 == 0 &&
            info.preferred_base <= std::numeric_limits<std::uint64_t>::max() - info.image_size,
            "invalid or overflowing preferred base");
    require(info.headers_size && info.headers_size % info.file_alignment == 0 &&
            info.headers_size <= info.image_size && info.headers_size <= data.size(), "invalid header size");
    auto subsystem = u(opt + 68, 2);
    supported(subsystem == 2 || subsystem == 3, "only Windows GUI/CUI subsystems supported");
    supported(!(u(opt + 70, 2) & 0x4000), "Control Flow Guard unsupported");
    auto number = u(opt + 108, 4);
    require(number <= (optional_size - 112) / 8, "truncated data directory array");
    supported(number <= 16, "extra data directories unsupported");
    for (unsigned i = 0; i < number; ++i) {
      directories[i] = {static_cast<Rva>(u(opt + 112 + i * 8, 4)),
                        static_cast<std::uint32_t>(u(opt + 116 + i * 8, 4))};
      require(bool(directories[i].rva) == bool(directories[i].size), "incomplete data directory");
    }
    auto table = opt + optional_size;
    require(range(table, count * 40, info.headers_size), "section table exceeds headers");
    for (unsigned i = 0; i < count; ++i) {
      auto p = table + i * 40;
      Section s;
      for (unsigned j = 0; j < 8 && u(p + j, 1); ++j) s.name += static_cast<char>(u(p + j, 1));
      s.virtual_size = static_cast<std::uint32_t>(u(p + 8, 4));
      s.rva = static_cast<Rva>(u(p + 12, 4));
      s.raw_size = static_cast<std::uint32_t>(u(p + 16, 4));
      s.file_offset = static_cast<std::uint32_t>(u(p + 20, 4));
      s.characteristics = static_cast<std::uint32_t>(u(p + 36, 4));
      auto extent = std::max(s.raw_size, s.virtual_size);
      require(extent && s.rva >= info.headers_size && s.rva % info.section_alignment == 0 &&
              range(s.rva, extent, info.image_size), "invalid section virtual range");
      require(!s.raw_size || (s.file_offset >= info.headers_size &&
              s.file_offset % info.file_alignment == 0 && s.raw_size % info.file_alignment == 0 &&
              range(s.file_offset, s.raw_size, data.size())), "invalid section raw range");
      supported((s.characteristics & (execute | write)) != (execute | write), "writable executable section unsupported");
      for (const auto& previous : info.sections) {
        require(!overlap(s.rva, extent, previous.rva, std::max(previous.raw_size, previous.virtual_size)),
                "overlapping virtual sections");
        require(!overlap(s.file_offset, s.raw_size, previous.file_offset, previous.raw_size),
                "overlapping raw sections");
      }
      info.sections.push_back(std::move(s));
    }
    auto entry_section = section(info.entry_rva, 1);
    require(entry_section && (entry_section->characteristics & execute), "entry is not in executable section");
    (void)offset(info.entry_rva, 1);
    for (unsigned i = 0; i < directories.size(); ++i) {
      auto d = directories[i];
      if (!d.size) continue;
      if (i == 4) require(range(d.rva, d.size, data.size()), "certificate file range outside file");
      else (void)offset(d.rva, d.size);
      switch (i) {
        case 1: case 3: case 5: case 6: case 12: break;
        case 9: fail("TLS directory unsupported", ErrorCode::unsupported_feature);
        case 13: fail("delay imports unsupported", ErrorCode::unsupported_feature);
        case 14: fail("CLR image unsupported", ErrorCode::unsupported_feature);
        case 10: fail("load configuration unsupported", ErrorCode::unsupported_feature);
        default: fail("data directory " + std::to_string(i) + " unsupported", ErrorCode::unsupported_feature);
      }
    }
    info.exception_directory = directories[3];
    require(info.exception_directory.size % 12 == 0, "truncated x64 runtime-function table");
    // Preserve the opaque directory; no unwind registration/dispatch is promised.
    imports();
    relocations();
  }
};
}
Result<Inspection> inspect(std::span<const std::byte> file) {
  try {
    supported(file.size() <= max_file, "file exceeds 64 MiB policy limit");
    auto result = std::make_unique<Snapshot>();
    result->data.assign(file.begin(), file.end());
    Parser(*result).parse();
    return Inspection(std::move(result));
  } catch (Diagnostic& error) {
    return std::move(error);
  } catch (const std::bad_alloc&) {
    Diagnostic error; error.code = ErrorCode::native_failure; error.event = "pe.inspect";
    error.unsupported_reason = "inspection allocation failed"; return error;
  }
}
}
