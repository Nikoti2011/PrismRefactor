"""Check externally visible worker termination and prefixed diagnostics."""
import subprocess
import sys

exe = sys.argv[1]
result = subprocess.run([exe, "--exit"], capture_output=True, text=True, timeout=10)
assert result.returncode == 0xAB, result
assert result.stderr == "[prism guest] ExitProcess code=305419947\n", result
result = subprocess.run([exe, "--debug"], capture_output=True, text=True, timeout=10)
assert result.returncode == 0, result
assert result.stderr == "[prism guest] fixture diagnostic\n", result
assert not result.stdout, result
print("PASS child exit DWORD diagnostic, POSIX low-byte status, debug output/error preservation")
