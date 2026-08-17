"""Rain splash from vanilla ExplosionSplash.nif - the game's own 'water up,
water falls back' effect.  Passes every donor rule learned this session:
plain NiNode root, no controller manager, direct-wired controllers, gravity
with a real object node (Gravity02), PLUS planar colliders so droplets die
at the ground plane.

Surgery:
 1. Orphan the grenade-scale water-column sheets (WideMesh/BaseMesh/FastMesh/
    TallMesh x6 + SurfaceFun) from the root children array - index-safe.
 2. Miniaturize both Drops emitters: cylinder r69.7->6 h75.9->2,
    speed 270/300 -> 120 (var scaled proportionally), life 6.0 -> 0.8.
 3. Gravity 180 -> 450 for a tight splash arc (FXDrips precedent: 900).
Textures (FluidDrops02Atlas) ship in vanilla BA2s - nothing to deploy.
"""
import struct

SRC=r'D:\Fallout 4 Meshes Source\Meshes\Effects\ExplosionSplash.nif'
DST=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\rainSplashSpark.nif'
DROP={2,15,25,35,48,61,99}   # mesh limb nodes to orphan

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

n=parse(SRC)
tn=lambda i:n['types'][n['tidx'][i]].decode()

# 1) orphan meshes from root children
root=n['blocks'][0]
o=4; ned,=struct.unpack_from('<I',root,o); o+=4+4*ned
o+=4+4+12+36+4+4
nch,=struct.unpack_from('<I',root,o)
ch=list(struct.unpack_from(f'<{nch}i',root,o+4))
keep=[c for c in ch if c not in DROP]
newroot=root[:o]+struct.pack('<I',len(keep))+b''.join(struct.pack('<i',c) for c in keep)
n['blocks'][0]=newroot
print(f'root children: {nch} -> {len(keep)}  kept={[(c,tn(c)) for c in keep]}')

# 2) emitters
for i in range(len(n['blocks'])):
    if tn(i)=='NiPSysCylinderEmitter':
        em=n['blocks'][i]
        sp,spv=struct.unpack_from('<2f',em,13)
        k=120.0/sp
        struct.pack_into('<2f',em,13,sp*k,spv*k)        # speed ratio preserved
        struct.pack_into('<2f',em,61,0.8,0.3)           # life
        struct.pack_into('<2f',em,73,6.0,2.0)           # radius, height
        print(f'  emitter[{i}]: speed {sp:.0f}->:{sp*k:.0f}(+/-{spv*k:.0f}) life 0.8 r=6 h=2')
    if tn(i)=='NiPSysGravityModifier':
        g=n['blocks'][i]
        struct.pack_into('<f',g,33,450.0)
        print(f'  gravity[{i}]: strength -> 450')

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
