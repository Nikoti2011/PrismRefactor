#pragma once
#include "prism/core.hpp"
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace prism::pe {
struct Section {
  std::string name;
  Rva rva{};
  std::uint32_t virtual_size{}, file_offset{}, raw_size{}, characteristics{};
};
struct Import { std::string dll, symbol; Rva iat_rva{}; };
struct Relocation { Rva rva{}; std::uint16_t type{}; }; // ABSOLUTE=0, DIR64=10
struct Directory { Rva rva{}; std::uint32_t size{}; };
struct ImageInfo {
  std::uint16_t machine{};
  GuestAddress preferred_base{};
  Rva entry_rva{};
  std::uint32_t image_size{}, headers_size{}, section_alignment{}, file_alignment{};
  bool relocations_stripped{};
  Directory exception_directory;
  std::vector<Section> sections;
  std::vector<Import> imports;
  std::vector<Relocation> relocations;
};
// Owns an immutable snapshot: inspection and loading must use the SAME bytes.
// Only the P03 implementation can construct this trusted object.
class InspectedImage {
public:
  virtual ~InspectedImage() = default;
  virtual const ImageInfo& info() const noexcept = 0;
  virtual std::span<const std::byte> bytes() const noexcept = 0;
protected:
  InspectedImage() = default;
};
using Inspection = std::unique_ptr<const InspectedImage>;
Result<Inspection> inspect(std::span<const std::byte> file);
// Resolver returns a nonzero process address of an explicitly guest-ABI adapter.
// DLL matching is ASCII case-insensitive; symbol matching is case-sensitive.
class ImportResolver {
public:
  virtual ~ImportResolver() = default;
  virtual Result<GuestAddress> resolve(std::string_view dll, std::string_view symbol) = 0;
};
// P05 concrete destructor releases reservation; sole ownership, no copying.
class MappedImage {
public:
  virtual ~MappedImage() = default;
  virtual GuestAddress base() const noexcept = 0;
  virtual std::uint64_t size() const noexcept = 0;
  virtual GuestAddress entry_address() const noexcept = 0;
protected:
  MappedImage() = default;
  MappedImage(const MappedImage&) = delete;
  MappedImage& operator=(const MappedImage&) = delete;
};
struct LoadOptions { bool force_relocation = false; };
Result<std::unique_ptr<MappedImage>> load(const InspectedImage&, ImportResolver&, LoadOptions = {});
}
