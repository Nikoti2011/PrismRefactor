#!/usr/bin/env python3
"""Build and inventory canonical fixtures; never executes Windows binaries."""
import argparse, hashlib, json, pathlib, re, shutil, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[2]
NAMES = ('probe', 'window', 'pong')
def tool(name):
    value = shutil.which('x86_64-w64-mingw32-' + name)
    if not value:
        value = '/opt/homebrew/opt/mingw-w64/bin/x86_64-w64-mingw32-' + name
    if not pathlib.Path(value).is_file():
        raise RuntimeError('Install a MinGW-w64 cross toolchain or put it on PATH: ' + name)
    return value
def run(args):
    return subprocess.check_output([str(a) for a in args], text=True, stderr=subprocess.STDOUT)
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def build(out):
    out.mkdir(parents=True, exist_ok=True)
    gcc, objdump = tool('gcc'), tool('objdump')
    versions = {n:run([tool(n),'--version']) for n in ('gcc','ld','objdump')}
    results = {}
    for name in NAMES:
        folder=out/name; folder.mkdir(exist_ok=True)
        exe=folder/('prism-'+name+'.exe')
        command=[gcc,'-std=c11','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
                 '-fno-stack-protector','-fno-ident','-nostdlib',
                 '-Wl,--entry,PrismEntry,--subsystem,windows,--no-insert-timestamp,--image-base,0x180000000,--section-alignment,16384',
                 str(ROOT/'fixtures/windows/core'/name/'main.c'),'-o',str(exe),'-lkernel32']
        if name!='probe': command+=['-luser32','-lgdi32']
        run(command)
        headers=run([objdump,'-x',exe]); disassembly=run([objdump,'-d',exe])
        (folder/'headers.txt').write_text(headers)
        (folder/'disassembly.txt').write_text(disassembly)
        imports={}; dll=None
        for line in headers.splitlines():
            match=re.search(r'DLL Name: (\S+)',line)
            if match: dll=match[1]; imports[dll]=[]
            match=re.match(r'\s+[0-9a-f]+\s+<none>\s+[0-9a-f]+\s+(\w+)\s*$',line)
            if dll and match: imports[dll].append(match[1])
        allowed_kernel={'OutputDebugStringA','GetModuleHandleW','QueryPerformanceCounter','QueryPerformanceFrequency','Sleep','ExitProcess'}
        allowed_user={'RegisterClassExW','CreateWindowExW','GetClientRect','ShowWindow','PeekMessageW','TranslateMessage','DispatchMessageW','DefWindowProcW','DestroyWindow','PostQuitMessage','GetDC','ReleaseDC'}
        allowed={'KERNEL32.dll':allowed_kernel,'USER32.dll':allowed_user,'GDI32.dll':{'StretchDIBits'}}
        assert imports and all(d in allowed and set(s)<=allowed[d] for d,s in imports.items()), imports
        expected={'KERNEL32.dll':{'OutputDebugStringA'}} if name=='probe' else allowed
        assert {d:set(v) for d,v in imports.items()}==expected, imports
        assert not re.search(r'%gs:|%fs:|__chkstk|__stack_chk|__security_cookie|\b(?:memcpy|memset|malloc|free)\b',disassembly), 'Unexpected generated helper/TLS'
        directories={}
        for line in headers.splitlines():
            m=re.match(r'Entry ([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+(.*)',line)
            if m: directories[m[1]]={'rva':int(m[2],16),'size':int(m[3],16),'description':m[4]}
        assert directories and all(d['size']==0 for k,d in directories.items() if int(k,16) not in (1,3,5,6,12)),directories
        if name=='probe':
            assert directories['5']['size']>0 and 'DIR64' in headers, 'Probe must exercise base relocation'
        assert not re.search(r'\b(?:HIGHLOW|HIGHADJ)\b',headers)
        result={'sha256':digest(exe),'imports':imports,'directories':directories,'command':command,'versions':versions,
                'entry':'PrismEntry / DWORD(void)','execution':'Not run on Windows or Prism runtime',
                'source_sha256':{str(p.relative_to(ROOT)):digest(p) for p in sorted((ROOT/'fixtures/windows/core').rglob('*')) if p.suffix in ('.c','.h')}}
        (folder/'manifest.json').write_text(json.dumps(result,indent=2)+'\n'); results[name]=result
        print(name, result['sha256'], imports)
    return results
if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('output',type=pathlib.Path)
    build(parser.parse_args().output.resolve())
