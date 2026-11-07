P02 canonical Windows fixtures — contract v1

Original project-owned C source, authored for Prism; no third-party game code.
Uses installed MinGW-w64 SDK headers/import libraries, not Wine. No repository
license has been selected; this report does not grant redistribution rights.

Build from repository root:
  python3 tools/fixtures/build.py build/p02/fixtures
Checks and repeat builds:
  cmake -S . -B build/p02 -DPRISM_BUILD_P02=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build/p02 --target check-p02 check-p00
Requires MinGW-w64 GCC/binutils on PATH (x86_64-w64-mingw32-*), or the installed
Homebrew prefix. No installation or download occurs. Known tested versions are
GCC 16.2.0 and GNU binutils 2.47. The inventory reader deliberately validates
this binutils import-row format; other formats fail the exact import-set check.

Each output subdirectory contains prism-NAME.exe, manifest.json, headers.txt,
and disassembly.txt. Manifest records exact argv, versions, hashes, imports,
directories and all fixture C/header source hashes. Frozen observed summaries
live beside each source as inventory.json. Generated binaries stay in build/.
Build twice into distinct directories and require byte equality. This establishes
repeatability on the named toolchain, not across arbitrary compiler versions.

All fixtures: AMD64 PE32+, DWORD PrismEntry(void), no startup/CRT/default runtime,
no dynamic DLL loading, TLS, security cookie, stack probe, exception handlers,
resources or load configuration. Compiler-produced .pdata/.xdata describe stack
frames, but no Windows exception/unwind handling is required by fixture logic.
The SDK declarations are checked for contract-v1 sizes/offsets by fixture.h.
No shared C++ ABI material is copied into a private replacement header.

Probe: emits 'Prism canonical probe: PASS\n' once using OutputDebugStringA and
returns 0x50415353. A volatile initialized absolute pointer requires one actual
DIR64 fixup (plus ABSOLUTE padding). P05 must force relocation and verify both
message and full return value, not just the low POSIX exit byte.

Window/Pong: one 320x240 borderless WS_POPUP at (100,100), fixed client size,
NULL module lookup, named class, no menu/icons/cursor/brush, no parent. Creation
requires WM_NCCREATE TRUE / WM_CREATE zero; default handling uses DefWindowProcW.
PeekMessageW(NULL,0,0,PM_REMOVE), key-only TranslateMessage and DispatchMessageW.
WM_CLOSE or Escape calls DestroyWindow; WM_DESTROY posts WM_QUIT. Return its code.
Focus loss clears both keys; no text/IME, scaling/resizing, multiple windows.
Top-down 32-bit BI_RGB BGRX, X=0, full 320x240 source/destination, DIB_RGB_COLORS,
SRCCOPY; StretchDIBits must return 240. GetDC/ReleaseDC each rendered frame.
QPC uses reported positive frequency, one simulation step per >=1/60s; Sleep(1).
No catch-up after stalls; this is a bounded fixture, not precise frame pacing.
QPC frequency may differ on reference Windows; runtime v1 returns 1e9.

Window image: upper-left red, upper-right green, lower-left blue, lower-right
white. Up adds a yellow top stripe, Down a cyan bottom stripe; key-up/focus loss
removes them. Continuous redraw permits exposure checks; backend retained repaint
must separately be verified by P04/P07. Borderless close can use system close or
Escape; there is no titlebar close button.
Pong: arrows move left paddle, right paddle is automatic, ball collides/scores,
score is 0..9 blocks wrapping to zero, R restarts, Escape exits. Static pixel
storage prevents large stack frames. Logic tests execute the same game.h on the
native host; they do NOT establish Windows PE execution, key delivery or drawing.

Expected errors: 10 class/module; 11 creation; 12 client bounds; 13 initial timer;
14 timer update; ExitProcess 20 DC, 21 presentation, 22 release. Ordinary entry
return relies on the host/Windows thread entry behavior; no CRT cleanup is used.

Reference Windows execution: unavailable/not performed. No Windows installation,
VM or Wine used. Prism PE execution and gameplay validation belong to P05/P07/P08;
P02 builds and inventories fixtures, not runtime adapters. Window/Pong have no
base relocations with this compiler (RIP-relative data and runtime callback
assignment); do not equate this with P05's forced DIR64 relocation test.
