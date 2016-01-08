/*
 * xrick/system/basic_funcs.c
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza. All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#include "xrick/system/basic_funcs.h"

#ifdef USE_DEFAULT_ENDIANNESS_FUNCTIONS

extern inline uint16_t swap16(uint16_t x);
extern inline uint32_t swap32(uint32_t x);

extern inline uint16_t htobe16(uint16_t host);
extern inline uint16_t htole16(uint16_t host);
extern inline uint16_t betoh16(uint16_t big_endian);
extern inline uint16_t letoh16(uint16_t little_endian);

extern inline uint32_t htobe32(uint32_t host);
extern inline uint32_t htole32(uint32_t host);
extern inline uint32_t betoh32(uint32_t big_endian);
extern inline uint32_t letoh32(uint32_t little_endian);

#endif /* USE_DEFAULT_ENDIANNESS_FUNCTIONS */

/* eof */


