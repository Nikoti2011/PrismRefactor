#!/usr/bin/env python3
"""Capture reproducible local P01 evidence; run after building check-p01."""
import hashlib
import json
import pathlib
import subprocess
import sys
from bootstrap import build

root = pathlib.Path(__file__).resolve().parents[2]
build_dir = pathlib.Path(sys.argv[1]).resolve()
evidence = build_dir/'evidence'
evidence.mkdir(exist_ok=True)
host = build_dir/'components/experiments/execution/prism_execution_probe'
fixture = build_dir/'components/experiments/execution/bootstrap/bootstrap.exe'
abi = build_dir/'components/tests/abi/prism_abi_test'
records = []

def run(name, args, allowed=(0,)):
    result = subprocess.run([str(a) for a in args],capture_output=True,text=True,timeout=30)
    (evidence/(name+'.txt')).write_text('$ '+ ' '.join(map(str,args))+'\n'+result.stdout+result.stderr+'\nexit='+str(result.returncode)+'\n')
    records.append({'check':name,'exit':result.returncode})
    if result.returncode not in allowed:
        raise RuntimeError(f'{name}: exit {result.returncode}; see evidence')
    return result

run('os',['sw_vers'])
run('uname',['uname','-a'])
run('clang',['clang','--version'])
run('cmake',['cmake','--version'])
run('file',['file',host,fixture])
# codesign reports unsigned with exit 1; record that as an observed configuration.
run('signing',['codesign','-dvvv','--entitlements',':-',host],(0,1))
run('pe-inspection',['xcrun','llvm-objdump','--private-headers','--disassemble',fixture])
run('pe-callback',['xcrun','llvm-objdump','--disassemble','--start-address=0x180004100','--stop-address=0x18000410d',fixture])
run('abi-disassembly',['xcrun','llvm-objdump','--disassemble',abi])
run('abi',[abi])
for i in range(20):
    result = run(f'execution-{i:02}',[host,fixture])
    if 'guest_return=0x50415353 import_calls=1 unmapped=1' not in result.stdout:
        raise RuntimeError('missing checked return')
actual = fixture.read_bytes()
if actual != build() or build() != build():
    raise RuntimeError('fixture recipe mismatch')
hashes = {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()
          for folder in ['src/abi','experiments/execution','tests/abi']
          for p in (root/folder).rglob('*') if p.is_file() and '__pycache__' not in str(p)}
(evidence/'summary.json').write_text(json.dumps({'checks':records,'repeated_execution_passes':20,
    'fixture_sha256':hashlib.sha256(actual).hexdigest(),'source_sha256':hashes},indent=2)+'\n')
print('PASS: 20 PE import/callback/return runs, ABI suite, deterministic fixture; evidence: '+str(evidence))
