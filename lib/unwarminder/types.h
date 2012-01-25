/***************************************************************************
 * ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
 *
 * This program is PUBLIC DOMAIN.
 * This means that there is no copyright and anyone is able to take a copy
 * for free and use it as they wish, with or without modifications, and in
 * any context, commercially or otherwise. The only limitation is that I
 * don't guarantee that the software is fit for any purpose or accept any
 * liability for it's use or misuse - this software is without warranty.
 **************************************************************************/
/** \file
 * Types common across the whole system.
 **************************************************************************/

#ifndef TYPES_H
#define TYPES_H

#define UPGRADE_ARM_STACK_UNWIND
#undef UNW_DEBUG

typedef unsigned char   Int8;
typedef unsigned short  Int16;
typedef unsigned int    Int32;


typedef signed char     SignedInt8;
typedef signed short    SignedInt16;
typedef signed int      SignedInt32;


typedef enum
{
    FALSE,
    TRUE
} Boolean;

#endif

/* END OF FILE */
