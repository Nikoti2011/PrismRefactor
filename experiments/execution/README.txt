P01 EXECUTION FEASIBILITY (contract v1)

Build from the repository root:
  cmake -S . -B build/p01 -DPRISM_BUILD_P01=ON -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_BUILD_TYPE=Release
  cmake --build build/p01 --target check-p01
  PYTHONDONTWRITEBYTECODE=1 python3 experiments/execution/verify.py build/p01

Requires macOS, CMake >=3.24, Apple Clang, Python 3, and an executable x86-64
host route. Uses already-installed Rosetta on Apple silicon; installs nothing.
No Wine, VM, Windows SDK/import library, LLD, or system security changes.
See docs/subprojects/P01.txt for actual configuration/results and limitations.

bootstrap.py encodes a tiny reviewed x86-64 PE32+ program directly, avoiding a
linker prerequisite for P01. This is P01's explicitly permitted private bootstrap,
not the P02 C fixture builder. Generated EXE/header live only under build/p01.
Timestamp is zero and build() has no environment-dependent input. Verify with
xcrun llvm-objdump --private-headers --disassemble <bootstrap.exe>.
A separate callback-range disassembly avoids mis-decoding zero padding as code.

Fixed layout: headers RVA 0; .text RVA 0x4000; .idata RVA 0x8000; image 0xc000.
Entry RVA 0x4000, callback RVA 0x4100, IAT slot RVA 0x8060. The sole import is
PRISM.DLL!PrismProbe6, a private test adapter with six uint64 arguments. Entry
passes 1..6; adapter computes their weighted sum (91), calls back into PE code
with 10,20,30,-4 (56), and returns 91 only if the callback succeeds. Entry checks
91 and returns 0x50415353; a mismatch returns zero. Host checks return, exactly
one import call, and successful unmap. This executes bytes from the PE, not a
native recompilation. The bootstrap has no compiler helpers, TLS, GS accesses,
CRT, dynamic imports, load configuration, or exception/unwind directory.
It has no absolute addresses or base relocations; it uses RIP-relative code.
Executing at a nonpreferred address here does NOT test DIR64 relocation.
Windows reference execution is untested (private PRISM.DLL has no Windows build).

probe.cpp deliberately accepts only a byte-for-byte match to its generated
bootstrap_bytes.hpp. It copies fixed known offsets, patches that single IAT
entry, and sets pages R/RX/R before execution. It is NOT a general PE parser,
loader, security boundary, or production worker. P03/P05 own those components.
Memory is anonymous RW during preparation, then RX only for code; never RWX.
No MAP_FIXED, MAP_JIT, signing, or entitlement is added. Host-page divisibility
is checked. Invalid bootstrap bytes reject before mapping. Guest faults terminate
this standalone test process; there is no Windows exception/unwind support.

src/abi contains reusable typed entry/window callback helpers; tests/abi contains
boundary tests and test-only assembly. Shared definitions stay in include/prism.
Consumers link prism::abi and include bridge.hpp; no shared contract was changed.
Callbacks/entry must already be validated and remain mapped for the whole call;
helpers do not manage image lifetime or validate arbitrary guest addresses.
No exception may cross these noexcept ABI boundaries.

verify.py stores command outputs, source hashes, fixture hash, disassembly and
20 repeated executions in build/p01/evidence. sysctl queries can be denied by
the execution sandbox; translated=-1 is unknown, not native execution. The
handoff separately records the authorized unsandboxed query (translated=1).

Primary references reviewed 2026-11-06:
https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
https://clang.llvm.org/docs/AttributeReference.html#ms-abi
https://developer.apple.com/videos/play/wwdc2019/703/
https://help.apple.com/xcode/mac/current/en.lproj/devf87a2ac8f.html
Rosetta overview URL was JS-only in the research browser; runtime detection is
measured locally. No claim about future OS availability is inferred from it.
