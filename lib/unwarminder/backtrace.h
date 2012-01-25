/***************************************************************************
 * ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
 *
 * This program is PUBLIC DOMAIN.
 * This means that there is no copyright and anyone is able to take a copy
 * for free and use it as they wish, with or without modifications, and in
 * any context, commercially or otherwise. The only limitation is that I
 * don't guarantee that the software is fit for any purpose or accept any
 * liability for it's use or misuse - this software is without warranty.
 ***************************************************************************
 * File Description:  Unwinder client that reads local memory.
 **************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

/***************************************************************************
 * Nested Includes
 ***************************************************************************/
#include "config.h"
#include "system.h"
#include "lcd.h"

#include <stdio.h>
#include "unwarminder.h"
#include "get_sp.h"
#include "gcc_extensions.h"

#if defined(SIM_CLIENT)
#error This file is not for the simulated unwinder client
#endif

/***************************************************************************
 * Typedefs
 ***************************************************************************/

/** Example structure for holding unwind results.
 */
typedef struct
{
    Int16 frameCount;
    Int32 address[32];
}
CliStack;

/***************************************************************************
 * Variables
 ***************************************************************************/

extern const UnwindCallbacks cliCallbacks;

void backtrace(int pcAddr, int spAddr, unsigned *line);

#endif


/* END OF FILE */
