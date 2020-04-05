//
//  global.h
//  Duke3D
//
//  Created by fabien sanglard on 12-12-17.
//  Copyright (c) 2012 fabien sanglard. All rights reserved.
//

#ifndef Duke3D_global_h
#define Duke3D_global_h

#include "SDL.h"

#define open rb->open
#define strlwr duke_strlwr
#define strupr duke_strupr
#define itoa duke_itoa
#define ltoa duke_ltoa
#define ultoa duke_ultoa

void FixFilePath(char  *filename);
int FindDistance3D(int ix, int iy, int iz);
void Shutdown(void);

#ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
	#define BIG_ENDIAN 4321
#endif

#ifdef ROCKBOX
#undef BYTE_ORDER
#ifdef ROCKBOX_LITTLE_ENDIAN
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN
#endif
#endif

#ifndef BYTE_ORDER
#error Please define your platform.
#endif

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define KeepShort IntelShort
#define SwapShort MotoShort
#define Keepint32_t IntelLong
#define Swapint32_t MotoLong
#else
#define KeepShort MotoShort
#define SwapShort IntelShort
#define Keepint32_t MotoLong
#define Swapint32_t IntelLong
#endif

int32_t MotoLong (int32_t l);
int32_t IntelLong (int32_t l);

void Error (int errorType, char  *error, ...);

#endif
