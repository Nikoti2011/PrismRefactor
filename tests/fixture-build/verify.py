#!/usr/bin/env python3
import importlib.util, pathlib, sys
sys.dont_write_bytecode=True
root=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('fixture_build',root/'tools/fixtures/build.py')
builder=importlib.util.module_from_spec(spec); spec.loader.exec_module(builder)
out=pathlib.Path(sys.argv[1]).resolve()
a=builder.build(out/'fixtures'); b=builder.build(out/'repeat')
for name in builder.NAMES:
    assert a[name]['sha256']==b[name]['sha256'], 'Non-repeatable '+name
print('PASS: three byte-identical independent builds; import, directory, relocation and helper checks')
