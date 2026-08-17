import struct, os
SRC=r'D:\Fallout 4 Meshes Source\Meshes\Effects\SparksUp.nif'
DST=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\rainSplashSpark.nif'
TEX=r'Textures\Effects\rainDroplet.dds'

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
    nstr,=struct.unpack_from('<I',b,o);o+=4; maxlen,=struct.unpack_from('<I',b,o);o+=4
    strings=[]
    for _ in range(nstr):
        ln,=struct.unpack_from('<I',b,o);o+=4;strings.append(b[o:o+ln]);o+=ln
    ng,=struct.unpack_from('<I',b,o);o+=4; groups=list(struct.unpack_from(f'<{ng}I',b,o));o+=4*ng
    blocks=[]
    for s in sizes: blocks.append(bytearray(b[o:o+s]));o+=s
    return dict(hdrline=hdrline,ver=ver,endian=endian,uv=uv,bs=bs,strs=strs,mfp=mfp,
                types=types,tidx=tidx,strings=strings,groups=groups,blocks=blocks,tail=b[o:])

n=parse(SRC)
tn=lambda i:n['types'][n['tidx'][i]].decode()

FLAGS1=0x80010000; FLAGS2=0x00000020
def edit_shader(blk):
    o=0; o+=4; ned,=struct.unpack_from('<I',blk,o); o+=4+4*ned; o+=4  # name,extra,ctrl
    struct.pack_into('<II',blk,o,FLAGS1,FLAGS2); o+=8                 # flags
    o+=16                                                            # uv
    slen,=struct.unpack_from('<I',blk,o)
    nb=TEX.encode()
    return blk[:o]+struct.pack('<I',len(nb))+nb+blk[o+4+slen:]

for i in range(len(n['blocks'])):
    t=tn(i); blk=n['blocks'][i]
    if t=='BSEffectShaderProperty':
        n['blocks'][i]=bytearray(edit_shader(blk))
    elif t=='NiPSysBoxEmitter':
        struct.pack_into('<f',blk,13,150.0)   # speed
        struct.pack_into('<f',blk,17,45.0)    # speed var
        struct.pack_into('<f',blk,53,2.5)     # size
        struct.pack_into('<f',blk,57,0.6)     # size var
        struct.pack_into('<f',blk,61,0.35)    # life
        struct.pack_into('<f',blk,65,0.25)    # life var
    elif t=='BSPSysSimpleColorModifier':
        struct.pack_into('<3f',blk,53,0.75,0.85,1.0)  # color1 RGB -> water blue-white

out=bytearray(); out+=n['hdrline']+b'\x0A'
out+=struct.pack('<I',n['ver'])+bytes([n['endian']])+struct.pack('<I',n['uv'])
out+=struct.pack('<I',len(n['blocks']))+struct.pack('<I',n['bs'])
for s in (n['strs'][0],n['strs'][1],n['strs'][2],n['mfp']):
    out+=bytes([len(s)+1])+s+b'\x00'
out+=struct.pack('<H',len(n['types']))
for t in n['types']: out+=struct.pack('<I',len(t))+t
for ti in n['tidx']: out+=struct.pack('<H',ti)
for blk in n['blocks']: out+=struct.pack('<I',len(blk))
out+=struct.pack('<I',len(n['strings']))+struct.pack('<I',max((len(s) for s in n['strings']),default=0))
for s in n['strings']: out+=struct.pack('<I',len(s))+s
out+=struct.pack('<I',len(n['groups']))
for g in n['groups']: out+=struct.pack('<I',g)
for blk in n['blocks']: out+=blk
out+=n['tail']
open(DST,'wb').write(out)
print(f'wrote {DST} ({len(out)} bytes, {len(n["blocks"])} blocks)')
