
#ifndef __FASTMEM_H__
#define __FASTMEM_H__


#include "defs.h"
#include "mem.h"


byte readb(int a);
void writeb(int a, byte b);
int readw(int a);
void writew(int a, int w);
byte readhi(int a);
void writehi(int a, byte b);
#if 0
byte readhi(int a);
void writehi(int a, byte b);
#endif


#endif
