// this is meant to resolve platform dependency

#ifndef _SCALAR_TYPES_H
#define _SCALAR_TYPES_H


#ifdef WIN32
#include <windows.h>
#define SLEEP Sleep
#define GET_LAST_ERR GetLastError
#endif
// ToDo: add stuff for Linux



#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned long
#endif

#ifndef bool
#define bool int
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif





#endif

