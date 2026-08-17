"""Moving-droplet rain splash on the PROVEN genericB chassis.

genericB demonstrably emits when spawned by RainSplashesF4SE (direct-wired
controllers, plain NiNode root).  Surgery, append-only so no ref renumbering:
  1. emitter: speed 0 -> 150 (+/-45), declination up, cone ~50deg,
     size 2.5 (+/-0.8), life 0.42 (+/-0.15)
  2. append NiPSysGravityModifier cloned from vanilla SparksUp (FO4-native
     bytes), patched: name->new string, order=6, target=psys(5), gravityObj=-1
  3. register modifier ref in NiParticleSystem list
  4. texture -> rainDroplet16x1.dds (16-frame elongating droplet, matches
     genericB's 16 subtexture offsets + BSPSysSubTexModifier animation)
"""
import struct

REF=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\Rain\rain_rainSplashGround_genericB.nif'
SPARK=r'D:\Fallout 4 Meshes Source\Meshes\Effects\SparksUp.nif'
DST=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\rainSplashSpark.nif'
TEX=r'Textures\Effects\rainDroplet16x1.dds'

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
    nstr,=struct.unpack_from('<I',b,o);o+=4; o+=4
    strings=[]
    for _ in range(nstr):
        ln,=struct.unpack_from('<I',b,o);o+=4;strings.append(b[o:o+ln]);o+=ln
    ng,=struct.unpack_from('<I',b,o);o+=4; groups=list(struct.unpack_from(f'<{ng}I',b,o));o+=4*ng
    blocks=[]
    for s in sizes: blocks.append(bytearray(b[o:o+s]));o+=s
    return dict(hdrline=hdrline,ver=ver,endian=endian,uv=uv,bs=bs,strs=strs,mfp=mfp,
                types=types,tidx=tidx,strings=strings,groups=groups,blocks=blocks,tail=b[o:])

n=parse(REF)
tn=lambda nn,i:nn['types'][nn['tidx'][i]].decode()

# --- donor gravity block from SparksUp
sp=parse(SPARK)
gi=[i for i in range(len(sp['blocks'])) if tn(sp,i)=='NiPSysGravityModifier'][0]
grav=bytearray(sp['blocks'][gi])
axis=struct.unpack_from('<3f',grav,17); dec,stren=struct.unpack_from('<2f',grav,29)
print(f'donor gravity: axis={axis} decay={dec} strength={stren} ({len(grav)} bytes)')

PSYS=[i for i in range(len(n['blocks'])) if tn(n,i)=='NiParticleSystem'][0]
# patch donor: name -> new string idx, order=6, target=PSYS, gravityObject=-1
namestr=b'NiPSysGravityModifier:6'
nsi=len(n['strings']); n['strings'].append(namestr)
struct.pack_into('<i',grav,0,nsi)      # name
struct.pack_into('<I',grav,4,6)        # order
struct.pack_into('<i',grav,8,PSYS)     # target = particle system
grav[12]=1                             # active
struct.pack_into('<i',grav,13,-1)      # gravity object = none (world axis)
struct.pack_into('<f',grav,33,450.0)   # strength

# append block: new type entry + block
gt=b'NiPSysGravityModifier'
if gt not in n['types']:
    n['types'].append(gt)
gtidx=n['types'].index(gt)
n['tidx'].append(gtidx)
n['blocks'].append(grav)
GRAVI=len(n['blocks'])-1

# register in psys modifier list (append ref, bump count)
ps=n['blocks'][PSYS]
o=4; ned,=struct.unpack_from('<I',ps,o); o+=4+4*ned
o+=4+4+12+36+4+4          # ctlr flags trs colobj
o+=16+4+4+4               # bound skin shader alpha
o+=8+8+4+1                # desc, fades, dataref, worldspace
nmod,=struct.unpack_from('<I',ps,o)
struct.pack_into('<I',ps,o,nmod+1)
n['blocks'][PSYS]=ps+struct.pack('<i',GRAVI)
print(f'psys[{PSYS}]: modifiers {nmod} -> {nmod+1}, gravity block index {GRAVI}')

# --- emitter: give it upward velocity + droplet size/life
EM=[i for i in range(len(n['blocks'])) if tn(n,i)=='NiPSysBoxEmitter'][0]
em=n['blocks'][EM]
struct.pack_into('<f',em,13,150.0)  # speed
struct.pack_into('<f',em,17,45.0)   # speed var
struct.pack_into('<f',em,21,0.0)    # declination (0 = up)
struct.pack_into('<f',em,25,0.85)   # declination var (~50deg cone)
struct.pack_into('<f',em,53,2.5)    # size
struct.pack_into('<f',em,57,0.8)    # size var
struct.pack_into('<f',em,61,0.42)   # lifespan
struct.pack_into('<f',em,65,0.15)   # lifespan var
print(f'emitter[{EM}] patched: speed 150+/-45, cone 50deg up, size 2.5, life 0.42')

# --- texture swap
SH=[i for i in range(len(n['blocks'])) if tn(n,i)=='BSEffectShaderProperty'][0]
blk=n['blocks'][SH]
o=0; o+=4; ned,=struct.unpack_from('<I',blk,o); o+=4+4*ned; o+=4+8+16
slen,=struct.unpack_from('<I',blk,o)
tb=TEX.encode()
n['blocks'][SH]=blk[:o]+struct.pack('<I',len(tb))+tb+blk[o+4+slen:]
print(f'shader[{SH}] texture -> {TEX}')

# --- emit
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
open(DST,'wb').write(out)
print(f'wrote {DST}: {len(n["blocks"])} blocks, {len(out)} bytes')
