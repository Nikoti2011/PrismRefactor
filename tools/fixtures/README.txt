P02 fixture tooling
build.py uses the installed MinGW-w64 toolchain to build the three canonical C
fixtures and records exact commands/versions, binary and source hashes, binutils
headers/disassembly, imports and data directories. It enforces exact import sets,
no unexpected directories/helpers/TLS accesses, and a DIR64-bearing probe.
Run from any directory, passing an output directory under build/p02.
No runtime adapter, PE loader or parallel parser implementation is included.
See fixtures/windows/core/README.txt for recipes and behavioral requirements.
