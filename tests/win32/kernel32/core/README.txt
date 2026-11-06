P06 kernel service and process checks

cmake -S . -B build/p06 -DPRISM_BUILD_P06=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/p06 --target check-p06
For real host execution of x64 ABI adapters, use a separate build directory with
-DCMAKE_OSX_ARCHITECTURES=x86_64. Requires an already available x64 execution
route. Native ARM64 tests require registration to reject guest adapters.

Core checks compare QPC deltas with host monotonic time, check finite Sleep and
signal interruption, successful last-error preservation and documented failures.
x64 checks call resolved ms_abi pointers. Python captures child stderr and checks
ExitProcess full DWORD diagnostic/low-byte exit status and debug output. These
are native host tests, NOT actual imported calls from a Windows PE.
See docs/subprojects/P06.txt for exact configurations and retained evidence.
