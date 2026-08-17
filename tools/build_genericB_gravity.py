"""THE build: genericB chassis (the ONLY base proven to render via this
plugin's spawn path) + one correctly-wired, correctly-ORDERED gravity.

Proven facts driving this:
 - build_moving_splash (genericB + velocity + gravity) RENDERED but gravity
   never applied -> its gravity was appended AFTER Position/BoundUpdate.
 - Every working vanilla file places force modifiers BEFORE Position in the
   psys modifier array.  This inserts gravity there.
 - gravityObject MUST be a real node (block 3, the emitter node); -1 = no-op.

Minimal on purpose: ONE gravity, no drag/turbulence, so if it arcs we KNOW
array-order was the gravity bug.  Texture = rainDroplet16x1 (already deployed).
"""
import struct

REF=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\Rain\rain_rainSplashGround_genericB.nif'
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

n=parse(REF)
tn=lambda i:n['types'][n['tidx'][i]].decode()
PSYS=[i for i in range(len(n['blocks'])) if tn(i)=='NiParticleSystem'][0]
EMITNODE=3

# build a fresh NiPSysGravityModifier (50 bytes), fully specified
# layout: name(4) order(4) target(4) active(1) gravObj(4) axis(12) decay(4)
#         strength(4) forceType(4) turbulence(4) turbScale(4) worldAligned(1)
nsi=len(n['strings']); n['strings'].append(b'GravityDown')
g=bytearray(50)
struct.pack_into('<i',g,0,nsi)         # name
struct.pack_into('<I',g,4,1000)        # order
struct.pack_into('<i',g,8,PSYS)        # target
g[12]=1                                 # active
struct.pack_into('<i',g,13,EMITNODE)   # gravity object (real node)
struct.pack_into('<3f',g,17,0.0,0.0,-1.0)  # axis: straight DOWN, world
struct.pack_into('<f',g,29,0.0)        # decay
struct.pack_into('<f',g,33,700.0)      # strength
struct.pack_into('<I',g,37,0)          # forceType = Planar
struct.pack_into('<f',g,41,0.0)        # turbulence
struct.pack_into('<f',g,45,1.0)        # turbulence scale
g[49]=1                                 # world aligned
gt=b'NiPSysGravityModifier'
if gt not in n['types']: n['types'].append(gt)
n['tidx'].append(n['types'].index(gt))
n['blocks'].append(g)
GRAVI=len(n['blocks'])-1

# insert into modifier array BEFORE Position/BoundUpdate
ps=n['blocks'][PSYS]
o=4; ned,=struct.unpack_from('<I',ps,o); o+=4+4*ned
o+=4+4+12+36+4+4+16+4+4+4+8+8+4+1
nmod,=struct.unpack_from('<I',ps,o)
old=list(struct.unpack_from(f'<{nmod}i',ps,o+4))
tail=[r for r in old if tn(r) in ('NiPSysPositionModifier','NiPSysBoundUpdateModifier')]
head=[r for r in old if r not in tail]
ordered=head+[GRAVI]+tail
n['blocks'][PSYS]=ps[:o]+struct.pack('<I',len(ordered))+b''.join(struct.pack('<i',r) for r in ordered)
print('array:', [tn(r).replace('NiPSys','').replace('BSPSys','').replace('Modifier','') for r in ordered])

# emitter velocity (up-cone) + droplet size/life
EM=[i for i in range(len(n['blocks'])) if tn(i)=='NiPSysBoxEmitter'][0]
em=n['blocks'][EM]
struct.pack_into('<f',em,13,140.0)  # speed
struct.pack_into('<f',em,17,50.0)   # speed var
struct.pack_into('<f',em,21,0.0)    # declination = up
struct.pack_into('<f',em,25,0.60)   # ~34deg cone
struct.pack_into('<f',em,53,1.8)    # size
struct.pack_into('<f',em,57,0.6)
struct.pack_into('<f',em,61,0.55)   # life (long enough to see the arc)
struct.pack_into('<f',em,65,0.15)

# texture
SH=[i for i in range(len(n['blocks'])) if tn(i)=='BSEffectShaderProperty'][0]
blk=n['blocks'][SH]
o=0; o+=4; ned,=struct.unpack_from('<I',blk,o); o+=4+4*ned; o+=4+8+16
slen,=struct.unpack_from('<I',blk,o)
n['blocks'][SH]=blk[:o]+struct.pack('<I',len(TEX))+TEX+blk[o+4+slen:]

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
print(f'wrote {DST} ({len(out)} bytes, {len(n["blocks"])} blocks) gravity DOWN 700 @ array pos before Position')
