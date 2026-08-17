"""Post-pass: activate all NiTimeControllers in a NIF (clear bit0 app-init,
set bit3 Active) so the effect auto-plays when spawned as a bare temp effect.
Root cause of the 2026-08-16 invisible-donor chain: choreographed effects
(explosions, addon systems) ship controllers with Active clear, expecting
their host system to enable them at play time."""
import struct, sys

def activate(path):
    b=bytearray(open(path,'rb').read())
    nl=b.index(0x0A); o=nl+1; o+=4+1+4
    nb,=struct.unpack_from('<I',b,o);o+=4;o+=4
    def ss(o):
        n=b[o];return o+1+n
    for _ in range(4): o=ss(o)
    nt,=struct.unpack_from('<H',b,o);o+=2; types=[]
    for _ in range(nt):
        ln,=struct.unpack_from('<I',b,o);o+=4;types.append(b[o:o+ln].decode());o+=ln
    tidx=list(struct.unpack_from(f'<{nb}H',b,o));o+=2*nb
    sizes=list(struct.unpack_from(f'<{nb}I',b,o));o+=4*nb
    nstr,=struct.unpack_from('<I',b,o);o+=4;o+=4
    for _ in range(nstr):
        ln,=struct.unpack_from('<I',b,o);o+=4;o+=ln
    ng,=struct.unpack_from('<I',b,o);o+=4;o+=4*ng
    changed=0
    for i in range(nb):
        t=types[tidx[i]]
        if ('Ctlr' in t or t.endswith('Controller')) and 'Interpolator' not in t:
            fl,=struct.unpack_from('<H',b,o+4)
            nf=(fl & ~0x0001) | 0x0008
            if nf!=fl:
                struct.pack_into('<H',b,o+4,nf)
                print(f'  [{i}] {t}: {fl:#06x} -> {nf:#06x}')
                changed+=1
        o+=sizes[i]
    open(path,'wb').write(b)
    print(f'{path}: {changed} controllers activated')

if __name__=='__main__':
    for p in sys.argv[1:]:
        activate(p)
