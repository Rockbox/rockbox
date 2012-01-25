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
 * File Description: Utility functions and glue for ARM unwinding sub-modules.
 **************************************************************************/

#define MODULE_NAME "UNWARM"

/***************************************************************************
 * Include Files
 **************************************************************************/

#include "types.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "unwarm.h"
#include "unwarmmem.h"

/***************************************************************************
 * Manifest Constants
 **************************************************************************/


/***************************************************************************
 * Type Definitions
 **************************************************************************/


/***************************************************************************
 * Variables
 **************************************************************************/


/***************************************************************************
 * Macros
 **************************************************************************/


/***************************************************************************
 * Local Functions
 **************************************************************************/


/***************************************************************************
 * Global Functions
 **************************************************************************/

#if defined(UNW_DEBUG)
/** Printf wrapper.
 * This is used such that alternative outputs for any output can be selected
 * by modification of this wrapper function.
 */
void UnwPrintf(const char *format, ...)
{
    va_list args;

    va_start( args, format );
    vprintf(format, args );
}
#endif

/** Invalidate all general purpose registers.
 */
void UnwInvalidateRegisterFile(RegData *regFile)
{
    Int8 t = 0;

    do
    {
        regFile[t].o = REG_VAL_INVALID;
        t++;
    }
    while(t < 13);

}


/** Initialise the data used for unwinding.
 */
void UnwInitState(UnwState * const       state,   /**< Pointer to structure to fill. */
                  const UnwindCallbacks *cb,      /**< Callbacks. */
                  void                  *rptData, /**< Data to pass to report function. */
                  Int32                  pcValue, /**< PC at which to start unwinding. */
                  Int32                  spValue) /**< SP at which to start unwinding. */
{
    UnwInvalidateRegisterFile(state->regData);

    /* Store the pointer to the callbacks */
    state->cb = cb;
    state->reportData = rptData;

    /* Setup the SP and PC */
    state->regData[13].v = spValue;
    state->regData[13].o = REG_VAL_FROM_CONST;
    state->regData[15].v = pcValue;
    state->regData[15].o = REG_VAL_FROM_CONST;

    UnwPrintd3("\nInitial: PC=0x%08x SP=0x%08x\n", pcValue, spValue);

    /* Invalidate all memory addresses */
    memset(state->memData.used, 0, sizeof(state->memData.used));
}


/** Call the report function to indicate some return address.
 * This returns the value of the report function, which if TRUE
 * indicates that unwinding may continue.
 */
Boolean UnwReportRetAddr(UnwState * const state, Int32 addr)
{
    /* Cast away const from reportData.
     *  The const is only to prevent the unw module modifying the data.
     */
    return state->cb->report((void *)state->reportData, addr);
}


/** Write some register to memory.
 * This will store some register and meta data onto the virtual stack.
 * The address for the write
 * \param state [in/out]  The unwinding state.
 * \param wAddr [in]  The address at which to write the data.
 * \param reg   [in]  The register to store.
 * \return TRUE if the write was successful, FALSE otherwise.
 */
Boolean UnwMemWriteRegister(UnwState * const      state,
                            const Int32           addr,
                            const RegData * const reg)
{
    return UnwMemHashWrite(&state->memData,
                           addr,
                           reg->v,
                           M_IsOriginValid(reg->o));
}

/** Read a register from memory.
 * This will read a register from memory, and setup the meta data.
 * If the register has been previously written to memory using
 * UnwMemWriteRegister, the local hash will be used to return the
 * value while respecting whether the data was valid or not.  If the
 * register was previously written and was invalid at that point,
 * REG_VAL_INVALID will be returned in *reg.
 * \param state [in]  The unwinding state.
 * \param addr  [in]  The address to read.
 * \param reg   [out] The result, containing the data value and the origin
 *                     which will be REG_VAL_FROM_MEMORY, or REG_VAL_INVALID.
 * \return TRUE if the address could be read and *reg has been filled in.
 *         FALSE is the data could not be read.
 */
Boolean UnwMemReadRegister(UnwState * const      state,
                           const Int32           addr,
                           RegData * const       reg)
{
    Boolean tracked;

    /* Check if the value can be found in the hash */
    if(UnwMemHashRead(&state->memData, addr, &reg->v, &tracked))
    {
        reg->o = tracked ? REG_VAL_FROM_MEMORY : REG_VAL_INVALID;
        return TRUE;
    }
    /* Not in the hash, so read from real memory */
    else if(state->cb->readW(addr, &reg->v))
    {
        reg->o = REG_VAL_FROM_MEMORY;
        return TRUE;
    }
    /* Not in the hash, and failed to read from memory */
    else
    {
        return FALSE;
    }
}

/* END OF FILE */
