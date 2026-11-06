P01 ABI and execution checks

Run check-p01 from the x86-64 configuration in experiments/execution/README.txt.
abi.cpp tests typed entry/callback values, signed and wide returns, mixed
integer/double arguments across register and stack slots in both directions.
registers.S tests raw entry RSP modulo 16, writes all 32 shadow bytes, and
checks Windows RBX/RBP/RDI/RSI/R12-R15 and XMM6-XMM15 across a guest-ABI adapter
calling a SysV function that deliberately clobbers SysV-volatile registers.
A bad guest that corrupts XMM15 verifies that the harness can fail. Native and
Windows entry stack alignment are checked separately (both expect RSP%16=8).
Tests do not use assert, so Release builds retain all checks.
reject.py verifies truncated/modified PE inputs fail before executable mapping.
The execution test performs a real PE -> native adapter -> PE callback round trip.
