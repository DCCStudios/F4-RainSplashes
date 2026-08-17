"""Convert Skyrim SE particle-effect NIFs (bsver 100) to Fallout 4 (bsver 130).

Verified against hex reversal of the target files (2026-08-15):
- NiPSysData, NiParticleSystem, emitters/modifiers/controllers: identical layouts.
- NiParticleSystem: transplant FO4 vertex descriptor, clear SSE-only flag bits,
  zero far/near fade distances (match working FO4 reference).
- BSEffectShaderProperty: FO4 layout = SSE + 3 empty texture strings + 1 float;
  shader flag ENUMS differ between games -> adopt FO4 reference flags
  (0x80010000 / 0x00000020, proven soft-alpha particle setup), keep SSE
  emissive/falloff/texture.
- Header: bsver 130 + 'Max Filepath' short string; add '' to string table for
  the FO4 shader property Name convention.
- BSXFlags: clear the Havok bit if set (no collision blocks present).
"""
import struct, sys, shutil

FO4_VERTEX_DESC = bytes.fromhex('31 00 00 02 00 20 40 08'.replace(' ', ''))
FO4_FLAGS1 = 0x80010000
FO4_FLAGS2 = 0x00000020

def short_string(b, o):
    n = b[o]
    return b[o+1:o+1+n], o + 1 + n

def parse(path):
    b = open(path, 'rb').read()
    nl = b.index(0x0A)
    o = nl + 1
    ver, = struct.unpack_from('<I', b, o); o += 4
    endian = b[o]; o += 1
    uv, = struct.unpack_from('<I', b, o); o += 4
    nblocks, = struct.unpack_from('<I', b, o); o += 4
    bsver, = struct.unpack_from('<I', b, o); o += 4
    author, o = short_string(b, o)
    proc, o = short_string(b, o)
    exp, o = short_string(b, o)
    assert bsver == 100, f'expected SSE bsver 100, got {bsver}'
    ntypes, = struct.unpack_from('<H', b, o); o += 2
    types = []
    for _ in range(ntypes):
        ln, = struct.unpack_from('<I', b, o); o += 4
        types.append(b[o:o+ln].decode('latin1')); o += ln
    tidx = list(struct.unpack_from(f'<{nblocks}H', b, o)); o += 2 * nblocks
    sizes = list(struct.unpack_from(f'<{nblocks}I', b, o)); o += 4 * nblocks
    nstr, = struct.unpack_from('<I', b, o); o += 4
    maxlen, = struct.unpack_from('<I', b, o); o += 4
    strings = []
    for _ in range(nstr):
        ln, = struct.unpack_from('<I', b, o); o += 4
        strings.append(b[o:o+ln]); o += ln
    ngroups, = struct.unpack_from('<I', b, o); o += 4
    groups = list(struct.unpack_from(f'<{ngroups}I', b, o)); o += 4 * ngroups
    blocks = []
    for s in sizes:
        blocks.append(bytearray(b[o:o+s])); o += s
    tail = b[o:]
    return dict(hdrline=b[:nl], ver=ver, endian=endian, uv=uv, author=author,
                proc=proc, exp=exp, types=types, tidx=tidx, strings=strings,
                groups=groups, blocks=blocks, tail=tail)

def convert_effect_shader(blk, empty_str_idx):
    """SSE BSEffectShaderProperty -> FO4 layout."""
    o = 0
    out = bytearray()
    o += 4                                              # Name (SSE: -1)
    out += struct.pack('<i', empty_str_idx)             # FO4 convention: ''
    ned, = struct.unpack_from('<I', blk, o)
    out += blk[o:o+4+4*ned]; o += 4 + 4*ned             # extra data list
    out += blk[o:o+4]; o += 4                           # controller
    o += 8                                              # SSE flags1/2 (enums differ)
    out += struct.pack('<II', FO4_FLAGS1, FO4_FLAGS2)
    out += blk[o:o+16]; o += 16                         # UV offset + scale
    slen, = struct.unpack_from('<I', blk, o)
    out += blk[o:o+4+slen]; o += 4 + slen               # source texture path
    o += 4                                              # clamp + 3 misc bytes
    out += bytes([blk[o-4], 0xFF, 0x00, 0x00])          # keep clamp, FO4 ref misc
    out += blk[o:o+24]; o += 24                         # falloffs(16) + emissive rgb+a start
    out += blk[o:o+16]; o += 16                         # emissive tail + multiple + soft depth
    glen, = struct.unpack_from('<I', blk, o)
    out += blk[o:o+4+glen]; o += 4 + glen               # greyscale texture
    assert o == len(blk), f'shader trailing bytes: {o} vs {len(blk)}'
    out += struct.pack('<I', 0) * 3                     # env map / normal / env mask (empty)
    out += struct.pack('<f', 0.01)                      # trailing float (FO4 ref value)
    return out

def patch_particle_system(blk):
    """Transplant FO4 vertex desc, clear SSE-only flag bits, zero fade dists."""
    o = 4                                               # name
    ned, = struct.unpack_from('<I', blk, o); o += 4 + 4*ned
    o += 4                                              # controller
    flags, = struct.unpack_from('<I', blk, o)
    struct.pack_into('<I', blk, o, flags & 0xFFFF)      # drop SSE-only high word
    o += 4
    o += 12 + 36 + 4 + 4                                # trs + collision object
    o += 16                                             # bound sphere
    o += 4 + 4 + 4                                      # skin, shader, alpha refs
    blk[o:o+8] = FO4_VERTEX_DESC; o += 8
    blk[o:o+8] = b'\x00' * 8; o += 8                    # far/near fade (match FO4 ref)
    return blk

def convert(src, dst):
    n = parse(src)
    empty_idx = len(n['strings'])
    n['strings'].append(b'')

    for i, blk in enumerate(n['blocks']):
        t = n['types'][n['tidx'][i]]
        if t == 'BSEffectShaderProperty':
            n['blocks'][i] = convert_effect_shader(blk, empty_idx)
            print(f'  [{i}] {t}: {len(blk)} -> {len(n["blocks"][i])} bytes, FO4 flags')
        elif t == 'NiParticleSystem':
            patch_particle_system(blk)
            print(f'  [{i}] {t}: vertex desc + flags patched')
        elif t == 'BSXFlags':
            val, = struct.unpack_from('<I', blk, 4)
            if val & 0x2:
                struct.pack_into('<I', blk, 4, val & ~0x2)
                print(f'  [{i}] BSXFlags: cleared Havok bit ({val:#x} -> {val & ~0x2:#x})')
            else:
                print(f'  [{i}] BSXFlags: {val:#x} (unchanged)')

    out = bytearray()
    out += n['hdrline'] + b'\x0A'
    out += struct.pack('<I', n['ver'])
    out += bytes([n['endian']])
    out += struct.pack('<I', n['uv'])
    out += struct.pack('<I', len(n['blocks']))
    out += struct.pack('<I', 130)                       # FO4 stream version
    for s in (n['author'], n['proc'], n['exp'], b''):   # + Max Filepath
        out += bytes([len(s) + 1]) + s + b'\x00'
    out += struct.pack('<H', len(n['types']))
    for t in n['types']:
        tb = t.encode('latin1')
        out += struct.pack('<I', len(tb)) + tb
    for ti in n['tidx']:
        out += struct.pack('<H', ti)
    for blk in n['blocks']:
        out += struct.pack('<I', len(blk))
    out += struct.pack('<I', len(n['strings']))
    out += struct.pack('<I', max((len(s) for s in n['strings']), default=0))
    for s in n['strings']:
        out += struct.pack('<I', len(s)) + s
    out += struct.pack('<I', len(n['groups']))
    for g in n['groups']:
        out += struct.pack('<I', g)
    for blk in n['blocks']:
        out += blk
    out += n['tail']

    shutil.copy2(src, src + '.sse.bak')
    open(dst, 'wb').write(out)
    print(f'  wrote {dst} ({len(out)} bytes), backup at {src}.sse.bak')

if __name__ == '__main__':
    for p in sys.argv[1:]:
        print(f'== converting {p}')
        convert(p, p)
