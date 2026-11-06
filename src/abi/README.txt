P01 reusable ABI helpers

bridge.hpp/bridge.cpp invoke the canonical v1 EntryPoint and WindowProc types.
Link prism::abi. Build only in an x86-64 macOS process; cross-architecture calls
are explicitly rejected. These are typed compiler-generated calling-convention
bridges, not an instruction translator, exception boundary, or address validator.
Tests and measured configuration: docs/subprojects/P01.txt.
