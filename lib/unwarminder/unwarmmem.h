/***************************************************************************
 * ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
 *
 * This program is PUBLIC DOMAIN.
 * This means that there is no copyright and anyone is able to take a copy
 * for free and use it as they wish, with or without modifications, and in
 * any context, commerically or otherwise. The only limitation is that I
 * don't guarantee that the software is fit for any purpose or accept any
 * liablity for it's use or misuse - this software is without warranty.
 ***************************************************************************
 * File Description: Interface to the memory tracking sub-system.
 **************************************************************************/

#ifndef UNWARMMEM_H
#define UNWARMMEM_H

/***************************************************************************
 * Nested Include Files
 **************************************************************************/

#include "types.h"
#include "unwarm.h"

/***************************************************************************
 * Manifest Constants
 **************************************************************************/


/***************************************************************************
 * Type Definitions
 **************************************************************************/


/***************************************************************************
 *  Macros
 **************************************************************************/


/***************************************************************************
 *  Function Prototypes
 **************************************************************************/

Boolean UnwMemHashRead  (MemData * const memData,
                         Int32           addr,
                         Int32   * const data,
                         Boolean * const tracked);

Boolean UnwMemHashWrite (MemData * const memData,
                         Int32           addr,
                         Int32           val,
                         Boolean         valValid);

void    UnwMemHashGC    (UnwState * const state);

#endif

/* END OF FILE */
