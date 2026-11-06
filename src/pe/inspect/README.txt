P03 — bounded PE32+ inspection (contract v1)

Target: prism::pe_inspect; public API: include/prism/pe.hpp, prism::pe::inspect.
Link the target and pass a borrowed byte span. Success returns a sole-owned const
InspectedImage with a copied immutable snapshot and owned metadata. P05 must use
that snapshot, not reopen its path. No executable allocation or guest call occurs.
Failure returns Diagnostic with event pe.inspect, classification and reason;
RVA/DLL context is supplied where available. Internal parsing diagnostics are
caught before returning; allocation failure becomes native_failure.

Supported structural subset:
  AMD64 PE32+, Windows GUI/CUI executable, 1-96 non-overlapping sections.
  Power-of-two file alignment 512..65536 and section alignment >=4096, >=file
  alignment. Headers, raw ranges, section extents and file-backed entry checked.
  Section mapped extent is max(VirtualSize, SizeOfRawData); zero tails preserved.
  Named, unbound imports; OriginalFirstThunk=0 uses IAT. ASCII DLL names become
  lowercase; symbol case is retained. IAT and lookup values must initially match.
  ABSOLUTE padding and DIR64 relocations, including targets in virtual zero tails.
  Overlapping DIR64 writes and duplicate IAT destinations are malformed.
  Missing/stripped relocations are metadata, not a guarantee of relocatability.
  Exception/debug directories are range-checked opaque data. x64 exception table
  size must be a multiple of 12; unwind semantics are NOT validated or implemented.
  IAT directory is range checked; named import descriptors define actual slots.

Explicit unsupported_feature policy:
  Guest DLL, PE32/non-AMD64, non-GUI/CUI subsystem, CFG, low section alignment,
  writable+executable sections, bound/ordinal/delay imports, TLS, CLR, load config.
  All nonempty directories except imports, exceptions, relocations, debug and IAT
  are rejected (including resources, exports, certificates and bound imports).
  Certificate offsets are file offsets, checked as such before policy rejection.
  64 MiB file, 256 MiB image, 1024 descriptors, 65536 imports/relocation records,
  65536 thunks per descriptor, 4096-byte import-name caps bound resource use.
  These caps/restrictions are the narrow Prism policy, not universal PE rules.
  A malformed bounds check may take precedence over unsupported classification.

No promise of executable-code safety, Windows exception support, dynamic API
inventory, arbitrary PE compatibility, or page-permission compatibility. P05
must check actual host page sharing, runtime addresses and mapping protections.
Unknown DLL/symbol resolution belongs to P06/P05, not this structural parser.

Format reference consulted 2026-11-06:
https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
Architecture authority: docs/decisions/2026-11-06-contract-v1.txt.
Build/check commands and actual evidence: docs/subprojects/P03.txt.
