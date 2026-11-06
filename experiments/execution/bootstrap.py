#!/usr/bin/env python3
"""Deterministic P01-only hand-encoded PE32+ bootstrap; no SDK/linker needed.
Not a general fixture builder. Microsoft PE layout; disassemble the output.
All code uses RIP-relative addressing, so no base relocations are necessary.
"""
import hashlib
import pathlib
import struct
import sys


def build():
    b = bytearray(0x600)
    def put(offset, fmt, *values):
        struct.pack_into('<' + fmt, b, offset, *values)
    b[:2] = b'MZ'
    put(0x3c, 'I', 0x80)
    b[0x80:0x84] = b'PE\0\0'
    put(0x84, 'HHIIIHH', 0x8664, 2, 0, 0, 0, 240, 0x22)
    o = 0x98
    put(o, 'HBBIIIII', 0x20b, 0, 0, 0x200, 0x200, 0, 0x4000, 0x4000)
    put(o+24, 'QII', 0x180000000, 0x4000, 0x200)
    put(o+40, 'HHHHHH', 6, 0, 0, 0, 6, 0)
    put(o+56, 'IIIHH', 0xc000, 0x200, 0, 3, 0x100)
    put(o+72, 'QQQQII', 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    put(o+112+8, 'II', 0x8000, 40)  # import descriptors, including null
    put(o+112+12*8, 'II', 0x8060, 16)  # IAT
    for pos, name, rva, raw, flags in [(o+240,b'.text',0x4000,0x200,0x60000020),
                                      (o+280,b'.idata',0x8000,0x400,0xc0000040)]:
        put(pos,'8sIIIIIIHHI',name,0x200,rva,0x200,raw,0,0,0,0,flags)
    code = bytearray.fromhex(
        '48 83 ec 38 '         # sub rsp,56: shadow, two stack args, alignment
        'b9 01 00 00 00 '      # ecx=1
        'ba 02 00 00 00 '      # edx=2
        '41 b8 03 00 00 00 '   # r8d=3
        '41 b9 04 00 00 00 '   # r9d=4
        '48 c7 44 24 20 05 00 00 00 '  # fifth=5
        '48 c7 44 24 28 06 00 00 00')  # sixth=6
    call_rva = 0x4000 + len(code)
    code += b'\xff\x15' + struct.pack('<i', 0x8060-(call_rva+6))
    code += bytes.fromhex('48 83 c4 38 48 83 f8 5b 75 06 b8 53 53 41 50 c3 31 c0 c3')
    b[0x200:0x200+len(code)] = code
    # Four-argument Windows callback: return hwnd+message+wparam+lparam.
    callback = bytes.fromhex('48 89 c8 48 01 d0 4c 01 c0 4c 01 c8 c3')
    b[0x300:0x300+len(callback)] = callback
    put(0x400,'IIIII',0x8040,0,0,0x8080,0x8060)
    put(0x440,'QQ',0x80a0,0)
    put(0x460,'QQ',0x80a0,0)
    b[0x480:0x480+10] = b'PRISM.DLL\0'
    b[0x4a2:0x4a2+12] = b'PrismProbe6\0'
    return bytes(b)

if __name__ == '__main__':
    out = pathlib.Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    data = build()
    (out/'bootstrap.exe').write_bytes(data)
    (out/'bootstrap_bytes.hpp').write_text(
        '#pragma once\ninline constexpr unsigned char bootstrap_bytes[] = {' +
        ','.join(str(x) for x in data) + '};\n')
    print('bootstrap.exe sha256=' + hashlib.sha256(data).hexdigest())
