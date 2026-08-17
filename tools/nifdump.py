"""Dump NIF header + block inventory (20.2.0.7 era: SSE=100, FO4=130)."""
import struct, sys

def short_string(b, o):
    n = b[o]
    return b[o+1:o+1+n].rstrip(b'\0').decode('latin1'), o + 1 + n

def dump(path):
    b = open(path, 'rb').read()
    nl = b.index(0x0A)
    hdr = b[:nl].decode('latin1')
    o = nl + 1
    ver, = struct.unpack_from('<I', b, o); o += 4
    endian = b[o]; o += 1
    uv, = struct.unpack_from('<I', b, o); o += 4
    nblocks, = struct.unpack_from('<I', b, o); o += 4
    bsver, = struct.unpack_from('<I', b, o); o += 4
    author, o = short_string(b, o)
    if bsver > 130:
        o += 4  # unknown int (FO76)
    proc, o = short_string(b, o)
    exp, o = short_string(b, o)
    if bsver == 130:
        maxfp, o = short_string(b, o)   # FO4 'Max Filepath'
    ntypes, = struct.unpack_from('<H', b, o); o += 2
    types = []
    for _ in range(ntypes):
        ln, = struct.unpack_from('<I', b, o); o += 4
        types.append(b[o:o+ln].decode('latin1')); o += ln
    tidx = list(struct.unpack_from(f'<{nblocks}H', b, o)); o += 2 * nblocks
    sizes = list(struct.unpack_from(f'<{nblocks}I', b, o)); o += 4 * nblocks
    nstr, = struct.unpack_from('<I', b, o); o += 4
    o += 4  # max string length
    strings = []
    for _ in range(nstr):
        ln, = struct.unpack_from('<I', b, o); o += 4
        strings.append(b[o:o+ln].decode('latin1')); o += ln
    ngroups, = struct.unpack_from('<I', b, o); o += 4
    o += 4 * ngroups
    print(f'== {path}')
    print(f'   {hdr} | ver={ver:08X} uv={uv} bsver={bsver} blocks={nblocks} author="{author}"')
    off = o
    for i in range(nblocks):
        print(f'   [{i:2}] {types[tidx[i]]:<38} size={sizes[i]:<6} @0x{off:X}')
        off += sizes[i]
    print('   strings:', strings)
    print('   data-end vs file-size:', off, '/', len(b))
    return dict(buf=b, body=o, types=types, tidx=tidx, sizes=sizes, strings=strings, bsver=bsver, nblocks=nblocks)

if __name__ == '__main__':
    for p in sys.argv[1:]:
        dump(p)
        print()
