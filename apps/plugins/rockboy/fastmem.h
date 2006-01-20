
#ifndef __FASTMEM_H__
#define __FASTMEM_H__


#include "defs.h"
#include "mem.h"


byte readb(int a) ICODE_ATTR;
void writeb(int a, byte b) ICODE_ATTR;
int readw(int a) ICODE_ATTR;
void writew(int a, int w) ICODE_ATTR;
byte readhi(int a) ICODE_ATTR;
void writehi(int a, byte b) ICODE_ATTR;
#if 0
byte readhi(int a);
void writehi(int a, byte b);
#endif


#endif
