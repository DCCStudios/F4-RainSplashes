"""Path A: build working FO4 splash NIFs from the known-good genericB container.
Only two safe changes: (1) repoint the BSEffectShaderProperty inline source
texture, (2) patch NiPSysBoxEmitter radius/lifespan floats (size-neutral).
Lifecycle blocks (behavior graph, update ctlr, emitter ctlr) are untouched, so
the effect still self-terminates exactly like vanilla."""
import struct, os

REF = r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects\Rain\rain_rainSplashGround_genericB.nif'
DSTDIR = r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Meshes\Effects'
TEXDIR = r'F:\Modlists\MagnumOpus\mods\RainSplashesF4SE\Textures\Effects'

def parse(path):
    b=open(path,'rb').read(); nl=b.index(0x0A); hdrline=b[:nl]; o=nl+1
    ver,=struct.unpack_from('<I',b,o);o+=4; endian=b[o];o+=1
    uv,=struct.unpack_from('<I',b,o);o+=4
    nb,=struct.unpack_from('<I',b,o);o+=4
    bs,=struct.unpack_from('<I',b,o);o+=4
    def ss(o):
        n=b[o];return b[o+1:o+1+n],o+1+n
    strs=[]
    for _ in range(3):
        s,o=ss(o); strs.append(s)
    mfp,o=ss(o)
    nt,=struct.unpack_from('<H',b,o);o+=2
    types=[]
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
    for s in sizes: blocks.append(bytearray(b[o:o+s])); o+=s
    tail=b[o:]
    return dict(hdrline=hdrline,ver=ver,endian=endian,uv=uv,bs=bs,author=strs[0],
                proc=strs[1],exp=strs[2],mfp=mfp,types=types,tidx=tidx,strings=strings,
                groups=groups,blocks=blocks,tail=tail)

def find_type(n,name):
    return [i for i in range(len(n['blocks'])) if n['types'][n['tidx'][i]]==name.encode()]

def set_shader_texture(blk, newpath):
    o=0; o+=4                                   # name
    ned,=struct.unpack_from('<I',blk,o); o+=4+4*ned
    o+=4                                        # controller
    o+=8                                        # flags1/2
    o+=16                                       # uv offset+scale
    slen,=struct.unpack_from('<I',blk,o)
    nb=newpath.encode()
    new=bytearray(blk[:o]); new+=struct.pack('<I',len(nb))+nb
    new+=blk[o+4+slen:]
    return new

def patch_emitter(blk, radius, radvar, life, lifevar):
    struct.pack_into('<4f', blk, 53, radius, radvar, life, lifevar)
    return blk

def build(dst_name, tex, emitter=None):
    n=parse(REF)
    si=find_type(n,'BSEffectShaderProperty')[0]
    n['blocks'][si]=set_shader_texture(n['blocks'][si], tex)
    ei=find_type(n,'NiPSysBoxEmitter')[0]
    if emitter: patch_emitter(n['blocks'][ei], *emitter)
    out=bytearray(); out+=n['hdrline']+b'\x0A'
    out+=struct.pack('<I',n['ver'])+bytes([n['endian']])+struct.pack('<I',n['uv'])
    out+=struct.pack('<I',len(n['blocks']))+struct.pack('<I',n['bs'])
    for s in (n['author'],n['proc'],n['exp'],n['mfp']):
        out+=bytes([len(s)+1])+s+b'\x00'
    out+=struct.pack('<H',len(n['types']))
    for t in n['types']: out+=struct.pack('<I',len(t))+t
    for ti in n['tidx']: out+=struct.pack('<H',ti)
    for blk in n['blocks']: out+=struct.pack('<I',len(blk))
    out+=struct.pack('<I',len(n['strings']))
    out+=struct.pack('<I',max((len(s) for s in n['strings']),default=0))
    for s in n['strings']: out+=struct.pack('<I',len(s))+s
    out+=struct.pack('<I',len(n['groups']))
    for g in n['groups']: out+=struct.pack('<I',g)
    for blk in n['blocks']: out+=blk
    out+=n['tail']
    dst=os.path.join(DSTDIR,dst_name)
    open(dst,'wb').write(out)
    print(f'  {dst_name}: tex={tex} emitter={emitter} -> {len(out)} bytes')

# rainSplash.nif = punchier "with spray": bright texture, bigger/longer particles
build('rainSplash.nif', r'Textures\Effects\rainsplash16x1_vis.dds',
      emitter=(6.0, 3.0, 0.6, 0.35))
# rainSplashNoSpray.nif = subtle: hi-fi texture, near-vanilla shape
build('rainSplashNoSpray.nif', r'Textures\Effects\rainsplash16x1_hifi.dds',
      emitter=(4.5, 2.0, 0.5, 0.33))

print('textures present:',
      os.path.exists(os.path.join(TEXDIR,'rainsplash16x1_vis.dds')),
      os.path.exists(os.path.join(TEXDIR,'rainsplash16x1_hifi.dds')))
