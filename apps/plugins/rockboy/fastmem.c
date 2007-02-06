

#include "rockmacros.h"
#include "fastmem.h"

byte readb(int a)
{
    byte *p = mbc.rmap[a>>12];
    if (p) return p[a];
    else return mem_read(a);
}

void writeb(int a, byte b)
{
    byte *p = mbc.wmap[a>>12];
    if (p) p[a] = b;
    else mem_write(a, b);
}

int readw(int a)
{
    if ((a+1) & 0xfff)
    {
        byte *p = mbc.rmap[a>>12];
        if (p)
        {
#ifdef ROCKBOX_LITTLE_ENDIAN
#ifndef ALLOW_UNALIGNED_IO
            if (a&1) return p[a] | (p[a+1]<<8);
#endif
            return *(word *)(p+a);
#else
            return p[a] | (p[a+1]<<8);
#endif
        }
    }
    return mem_read(a) | (mem_read(a+1)<<8);
}

void writew(int a, int w)
{
    if ((a+1) & 0xfff)
    {
        byte *p = mbc.wmap[a>>12];
        if (p)
        {
#ifdef ROCKBOX_LITTLE_ENDIAN
#ifndef ALLOW_UNALIGNED_IO
            if (a&1)
            {
                p[a] = w;
                p[a+1] = w >> 8;
                return;
            }
#endif
            *(word *)(p+a) = w;
            return;
#else
            p[a] = w;
            p[a+1] = w >> 8;
            return;
#endif
        }
    }
    mem_write(a, w);
    mem_write(a+1, w>>8);
}

byte readhi(int a)
{
    return readb(a | 0xff00);
}

void writehi(int a, byte b)
{
    writeb(a | 0xff00, b);
}
