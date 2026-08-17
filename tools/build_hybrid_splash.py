"""Ballistic rain splash v3: genericB chassis (PROVEN to emit when spawned by
RainSplashesF4SE) + EPO MPSHeavyImpactSparks physics rig (PROVEN gravity/drag),
with the wiring mistake of v1 fixed: every modifier's object ref points at a
real node (genericB's emitter node, block 3) ??? gravityObject=-1 is a silent
no-op (learned 2026-08-16).

Grafted from EPO verbatim then patched (name/order/target/object):
  gravity 360 worldAligned + turbulence-gravity 9 + drag x3
Emitter: base floats authoritative on this chassis (no speed/radius ctlrs):
  speed 144+/-60 up-cone 40deg, size 1.5+/-0.5, life 0.5+/-0.25
Texture: rainDroplet16x1.dds (genericB subtex=16, already deployed).
"""
import struct

REF=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\Rain\rain_rainSplashGround_genericB.nif'
EPO=r'F:\Modlists\MagnumOpus\mods\Extreme Particles Overhaul\MESHES\Effects\MPSHeavyImpactSparks.nif'
DST=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\rainSplashSpark.nif'
TEX=rb'Textures\Effects\rainDroplet16x1.dds'

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

n=parse(REF); e=parse(EPO)
tn=lambda nn,i:nn['types'][nn['tidx'][i]].decode()

PSYS=[i for i in range(len(n['blocks'])) if tn(n,i)=='NiParticleSystem'][0]
EMITNODE=3   # genericB 'PCloud_rainSplashA-Emitter' NiNode

def graft(src_idx, name, order):
    blk=bytearray(e['blocks'][src_idx])
    t=e['types'][e['tidx'][src_idx]]
    nsi=len(n['strings']); n['strings'].append(name.encode())
    struct.pack_into('<i',blk,0,nsi)       # name
    struct.pack_into('<I',blk,4,order)     # order
    struct.pack_into('<i',blk,8,PSYS)      # target
    blk[12]=1                              # active
    struct.pack_into('<i',blk,13,EMITNODE) # gravity/drag object -> real node
    if t not in n['types']: n['types'].append(t)
    n['tidx'].append(n['types'].index(t))
    n['blocks'].append(blk)
    return len(n['blocks'])-1

# EPO blocks: 39 main gravity, 35 turbulence gravity, 36/37/38 drag XYZ
new_refs=[]
new_refs.append(graft(39,'NiPSysGravityModifier:9',4000))
new_refs.append(graft(35,'NiPSysGravityModifier:10',4001))
new_refs.append(graft(36,'NiPSysDragModifier:11',4002))
new_refs.append(graft(37,'NiPSysDragModifier:12',4003))
new_refs.append(graft(38,'NiPSysDragModifier:13',4004))
# make sure main gravity is world-aligned up-axis (copied verbatim = already 1)
g=n['blocks'][new_refs[0]]
struct.pack_into('<f',g,33,700.0)   # crisper fall (FXDrips=900 @ speed 30)
print(f'main gravity: axis={struct.unpack_from("<3f",g,17)} strength={struct.unpack_from("<f",g,33)[0]} worldAligned={g[49]}')

# register in psys modifier list - forces must sit BEFORE Position/BoundUpdate
# in the ARRAY (canonical layout of every working file: EPO, SparksUp, FXDrips)
ps=n['blocks'][PSYS]
o=4; ned,=struct.unpack_from('<I',ps,o); o+=4+4*ned
o+=4+4+12+36+4+4+16+4+4+4+8+8+4+1
nmod,=struct.unpack_from('<I',ps,o)
old_refs=list(struct.unpack_from(f'<{nmod}i',ps,o+4))
tail_refs=[r for r in old_refs if tn(n,r) in ('NiPSysPositionModifier','NiPSysBoundUpdateModifier')]
head_refs=[r for r in old_refs if r not in tail_refs]
ordered=head_refs+new_refs+tail_refs
struct.pack_into('<I',ps,o,len(ordered))
n['blocks'][PSYS]=ps[:o+4]+b''.join(struct.pack('<i',r) for r in ordered)
print(f'psys modifiers: {nmod} -> {len(ordered)}, forces inserted before Position/BoundUpdate')

# emitter
EM=[i for i in range(len(n['blocks'])) if tn(n,i)=='NiPSysBoxEmitter'][0]
em=n['blocks'][EM]
struct.pack_into('<f',em,13,144.0); struct.pack_into('<f',em,17,60.0)
struct.pack_into('<f',em,21,0.0);   struct.pack_into('<f',em,25,0.70)
struct.pack_into('<f',em,53,1.5);   struct.pack_into('<f',em,57,0.5)
struct.pack_into('<f',em,61,0.5);   struct.pack_into('<f',em,65,0.25)
print('emitter: speed 144+/-60, 40deg up-cone, size 1.5, life 0.5')

# texture
SH=[i for i in range(len(n['blocks'])) if tn(n,i)=='BSEffectShaderProperty'][0]
blk=n['blocks'][SH]
o=0; o+=4; ned,=struct.unpack_from('<I',blk,o); o+=4+4*ned; o+=4+8+16
slen,=struct.unpack_from('<I',blk,o)
n['blocks'][SH]=blk[:o]+struct.pack('<I',len(TEX))+TEX+blk[o+4+slen:]
print('texture -> rainDroplet16x1.dds')

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
for g2 in n['groups']: out+=struct.pack('<I',g2)
for blk in n['blocks']: out+=blk
out+=n['tail']
open(DST,'wb').write(out)
print(f'wrote {DST} ({len(out)} bytes, {len(n["blocks"])} blocks)')
