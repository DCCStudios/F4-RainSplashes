"""Water splash from EPO's MPSHeavyImpactSparks (user-directed donor).

Physics rig untouched (gravity x2 incl. proper gravityObject node wiring,
drag x3, turbulence).  Identity changes only:
  - speed curve x0.12 (1200 -> ~144), radius curve x2 (0.75 -> ~1.5),
    birth-rate curve x0.12 (750/s -> ~90/s), emitter base floats to match
  - lifespan 1.0+/-0.67 -> 0.5+/-0.25, cone 25deg -> 40deg
  - shader: source -> rainDroplet2x1.dds, orange gradient CLEARED,
    flags -> soft-alpha 0x80010000/0x20 (genericB-proven)
  - color keys: white -> blue-white, ember-red fade -> watery blue
"""
import struct

SRC=r'F:\Modlists\MagnumOpus\mods\Extreme Particles Overhaul\MESHES\Effects\MPSHeavyImpactSparks.nif'
DST=r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\rainSplashSpark.nif'
TEX=rb'Textures\Effects\rainDroplet2x1.dds'
SPEED_K=0.12; RAD_K=2.0; BIRTH_K=0.12

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

def scale_floatdata(blk,k):
    nk,=struct.unpack_from('<I',blk,0)
    ksz=(len(blk)-8)//nk
    for i in range(nk):
        off=8+i*ksz
        vals=list(struct.unpack_from(f'<{ksz//4}f',blk,off))
        # vals[0]=time, rest are value(+tangents) -> scale all but time
        for j in range(1,len(vals)): vals[j]*=k
        struct.pack_into(f'<{ksz//4}f',blk,off,*vals)

# identify float datas by their controller chains (decoded earlier):
# [13] birth rate (linear), [17] speed (quad), [19] radius (quad)
fds=[i for i in range(len(n['blocks'])) if tn(i)=='NiFloatData']
for i in fds:
    blk=n['blocks'][i]
    v1,=struct.unpack_from('<f',blk,12)   # first key value
    if v1>400 and len(blk)==88:   scale_floatdata(blk,SPEED_K); print(f'  [{i}] speed curve x{SPEED_K}')
    elif v1>400:                  scale_floatdata(blk,BIRTH_K); print(f'  [{i}] birth curve x{BIRTH_K}')
    else:                         scale_floatdata(blk,RAD_K);   print(f'  [{i}] radius curve x{RAD_K}')

EM=[i for i in range(len(n['blocks'])) if tn(i)=='NiPSysBoxEmitter'][0]
em=n['blocks'][EM]
struct.pack_into('<f',em,13,144.0)  # base speed
struct.pack_into('<f',em,17,60.0)
struct.pack_into('<f',em,25,0.70)   # cone ~40deg
struct.pack_into('<f',em,53,1.5)    # size
struct.pack_into('<f',em,57,0.5)
struct.pack_into('<f',em,61,0.5)    # life
struct.pack_into('<f',em,65,0.25)
print(f'  emitter[{EM}]: speed 144+/-60, cone 40deg, size 1.5, life 0.5')

# color modifier: keep alpha envelope, water tint
CM=[i for i in range(len(n['blocks'])) if tn(i)=='BSPSysSimpleColorModifier'][0]
cm=n['blocks'][CM]
struct.pack_into('<4f',cm,37,0.95,0.97,1.0,0.0)   # c0 birth
struct.pack_into('<4f',cm,53,0.85,0.92,1.0,1.0)   # c1 mid
struct.pack_into('<4f',cm,69,0.55,0.65,0.80,0.0)  # c2 fade
print(f'  color[{CM}]: water blue-white')

# shader rebuild
SH=[i for i in range(len(n['blocks'])) if tn(i)=='BSEffectShaderProperty'][0]
d=n['blocks'][SH]
o=0; o+=4; ned,=struct.unpack_from('<I',d,o); o+=4+4*ned; ctrl_end=o+4
head=bytes(d[:ctrl_end])
o=ctrl_end
o+=8   # old flags
uv=bytes(d[o:o+16]); o+=16
slen,=struct.unpack_from('<I',d,o); o+=4+slen        # old source tex
clamp4=bytes(d[o:o+4]); o+=4
falloff_emis=bytes(d[o:o+40]); o+=40                 # falloffs+emissive+mult+soft
glen,=struct.unpack_from('<I',d,o); o+=4+glen        # old greyscale gradient
tail=bytes(d[o:])                                    # env/norm/envmask/scale
new=bytearray()
new+=head
new+=struct.pack('<II',0x80010000,0x00000020)
new+=uv
new+=struct.pack('<I',len(TEX))+TEX
new+=clamp4
new+=falloff_emis
new+=struct.pack('<I',0)     # greyscale gradient CLEARED
new+=tail
n['blocks'][SH]=new
print(f'  shader[{SH}]: {len(d)} -> {len(new)} bytes, droplet tex, gradient cleared, soft-alpha flags')

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
print(f'wrote {DST} ({len(out)} bytes)')
