P02 checks
cmake -S . -B build/p02 -DPRISM_BUILD_P02=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/p02 --target check-p02 check-p00

reproducible builds all three fixtures twice in distinct paths, inventories their
linked requirements and compares full binary SHA-256 values. SDK ABI assertions
compile in window/Pong. game_logic checks paddle limits, wall/paddle collision,
both scoring directions, restart and released input, then 36000 deterministic
steps with position/score bounds. Native logic evidence is not guest execution.

Optional existing P03 integration check (does not edit P03):
build/p03/components/tests/pe/inspect/prism_pe_inspect_tests build/p02/fixtures/probe/prism-probe.exe
Repeat for window/prism-window.exe and pong/prism-pong.exe. See P03's README to
build its owned inspection target if unavailable. It only inspects bytes.
