P03 inspector verification

cmake -S . -B build/p03 -DPRISM_BUILD_P03=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/p03 --target check-p03

inspect_tests.cpp creates P03-only structural bytes in memory (not executable
fixture code and not a copy of P01/P02). It checks independently stated metadata,
immutable ownership, policy diagnostics, malformed headers/sections/imports/
relocations, every truncated prefix, and 20000 fixed-seed byte mutations. Checks
remain enabled in Release. There is no guest execution or executable mapping.

Sanitizer configuration:
cmake -S . -B build/p03-sanitize -DPRISM_BUILD_P03=ON -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
cmake --build build/p03-sanitize --target check-p03

The test binary can also inspect one external file and print metadata:
build/p03/components/tests/pe/inspect/prism_pe_inspect_tests PATH_TO_EXE
Exit 0 = accepted structurally; 2 = parser rejection; 1 = test/input failure.
No path is reopened by the library. The convenience test reader is not the
production launcher's bounded file reader; P05 owns that lifecycle.

Reuse external fixtures at their canonical path. For the existing private P01
bootstrap, generate without editing its source:
python3 experiments/execution/bootstrap.py build/p03/p01-bootstrap
build/p03/components/tests/pe/inspect/prism_pe_inspect_tests build/p03/p01-bootstrap/bootstrap.exe
P02's canonical C fixtures must be checked by the coordinator when delivered.
Generated evidence stays under build/p03 or build/p03-sanitize.
