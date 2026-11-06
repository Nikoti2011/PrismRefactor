import pathlib
import subprocess
import sys
import tempfile

host, fixture = sys.argv[1:]
data = pathlib.Path(fixture).read_bytes()
with tempfile.TemporaryDirectory() as tmp:
    for label, value in [('truncated', data[:-1]), ('changed-code', data[:512]+b'\xcc'+data[513:])]:
        path = pathlib.Path(tmp)/label
        path.write_bytes(value)
        result = subprocess.run([host,str(path)],capture_output=True,text=True,timeout=10)
        assert result.returncode == 3 and 'no mapping/entry' in result.stderr, result
        print(label + ': rejected before mapping')
