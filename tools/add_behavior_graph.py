"""Graft genericB's animation plumbing onto a NIF: BSBehaviorGraphExtraData
-> GenericBehaviors\StagesNoLoops\StagesNoLoops.hkx + BSXFlags 0x1.
The behavior graph is what CLOCKS a model's controllers when it is attached
outside a choreographing system (explosions/addons drive animation
externally; a bare temp-effect spawn has no conductor without BGED)."""
import struct, sys

HKX=b'GenericBehaviors\StagesNoLoops\StagesNoLoops.hkx'

def parse(path):
    b=open(path,'rb').read(); nl=b.index(0x0A); hdrline=b[:nl]; o=nl+1
    ver,=struct.unpack_from('<I',b,o);o+=4; endian=b[o];o+=1
    uv,=struct.unpack_from('<I',b,o);o+=4
    nb,=struct.unpack_from('<I',b,o);o+=4; bs,=struct.unpack_from('<I',b,o);o+=4
    def ss(o):
        n=b[o];return b[o+1:o+1+n],o+1+n
    strs=[]
    for _ in range(3): s,o=ss(o); strs.append(s)
    mfp,o=ss(o)
    nt,=struct.unpack_from('<H',b,o);o+=2; types=[]
    for _ in range(nt):
        ln,=struct.unpack_from('<I',b,o);o+=4;types.append(b[o:o+ln]);o+=ln
    tidx=list(struct.unpack_from(f'<{nb}H',b,o));o+=2*nb
    sizes=list(struct.unpack_from(f'<{nb}I',b,o));o+=4*nb
    nstr,=struct.unpack_from('<I',b,o);o+=4;o+=4
    strings=[]
    for _ in range(nstr):
        ln,=struct.unpack_from('<I',b,o);o+=4;strings.append(b[o:o+ln]);o+=ln
    ng,=struct.unpack_from('<I',b,o);o+=4; groups=list(struct.unpack_from(f'<{ng}I',b,o));o+=4*ng
    blocks=[]
    for s in sizes: blocks.append(bytearray(b[o:o+s]));o+=s
    return dict(hdrline=hdrline,ver=ver,endian=endian,uv=uv,bs=bs,strs=strs,mfp=mfp,
                types=types,tidx=tidx,strings=strings,groups=groups,blocks=blocks,tail=b[o:])

def emit(n,path):
    out=bytearray(); out+=n['hdrline']+b'\x0A'
    out+=struct.pack('<I',n['ver'])+bytes([n['endian']])+struct.pack('<I',n['uv'])
    out+=struct.pack('<I',len(n['blocks']))+struct.pack('<I',n['bs'])
    for s in (n['strs'][0],n['strs'][1],n['strs'][2],n['mfp']):
        out+=bytes([len(s)+1])+s+b'\x00'
    out+=struct.pack('<H',len(n['types']))
    for t in n['types']: out+=struct.pack('<I',len(t))+t
    for ti in n['tidx']: out+=struct.pack('<H',ti)
    for blk in n['blocks']: out+=struct.pack('<I',len(blk))
    out+=struct.pack('<I',len(n['strings']))+struct.pack('<I',max(len(s) for s in n['strings']))
    for s in n['strings']: out+=struct.pack('<I',len(s))+s
    out+=struct.pack('<I',len(n['groups']))
    for g in n['groups']: out+=struct.pack('<I',g)
    for blk in n['blocks']: out+=blk
    out+=n['tail']
    open(path,'wb').write(out)

def add_bged(path):
    n=parse(path)
    tn=lambda i:n['types'][n['tidx'][i]].decode()
    if any(tn(i)=='BSBehaviorGraphExtraData' for i in range(len(n['blocks']))):
        print(f'{path}: BGED already present'); return
    si_name=len(n['strings']); n['strings'].append(b'BGED')
    si_file=len(n['strings']); n['strings'].append(HKX)
    bged=struct.pack('<ii',si_name,si_file)+b'\x00'
    if b'BSBehaviorGraphExtraData' not in n['types']:
        n['types'].append(b'BSBehaviorGraphExtraData')
    n['tidx'].append(n['types'].index(b'BSBehaviorGraphExtraData'))
    n['blocks'].append(bytearray(bged))
    bi=len(n['blocks'])-1
    # root extra data list: BGED first (match genericB)
    root=n['blocks'][0]
    ned,=struct.unpack_from('<I',root,4)
    eds=list(struct.unpack_from(f'<{ned}i',root,8))
    n['blocks'][0]=root[:4]+struct.pack('<I',ned+1)+struct.pack('<i',bi)+root[8:]
    # BSXFlags -> 0x1 (Animated), matching genericB exactly
    for i in range(len(n['blocks'])):
        if tn(i)=='BSXFlags':
            v,=struct.unpack_from('<I',n['blocks'][i],4)
            struct.pack_into('<I',n['blocks'][i],4,0x1)
            print(f'  BSXFlags {v:#x} -> 0x1')
    emit(n,path)
    print(f'{path}: BGED block [{bi}] added, root extraData {ned}->{ned+1}')

if __name__=='__main__':
    for p in sys.argv[1:]: add_bged(p)
