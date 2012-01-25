/***************************************************************************
 * ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
 *
 * This program is PUBLIC DOMAIN.
 * This means that there is no copyright and anyone is able to take a copy
 * for free and use it as they wish, with or without modifications, and in
 * any context, commerically or otherwise. The only limitation is that I
 * don't guarantee that the software is fit for any purpose or accept any
 * liablity for it's use or misuse - this software is without warranty.
 **************************************************************************/
/** \file
 * Interface to the ARM stack unwinding module.
 **************************************************************************/

#ifndef UNWARMINDER_H
#define UNWARMINDER_H

/***************************************************************************
 * Nested Include Files
 **************************************************************************/

#include "types.h"

/***************************************************************************
 * Manifest Constants
 **************************************************************************/

/** \def UNW_DEBUG
 * If this define is set, additional information will be produced while
 * unwinding the stack to allow debug of the unwind module itself.
 */
/* #define UNW_DEBUG 1 */

/***************************************************************************
 * Type Definitions
 **************************************************************************/

/** Possible results for UnwindStart to return.
 */
typedef enum UnwResultTag
{
    /** Unwinding was successful and complete. */
    UNWIND_SUCCESS = 0,

    /** More than UNW_MAX_INSTR_COUNT instructions were interpreted. */
    UNWIND_EXHAUSTED,

    /** Unwinding stopped because the reporting func returned FALSE. */
    UNWIND_TRUNCATED,

    /** Read data was found to be inconsistent. */
    UNWIND_INCONSISTENT,

    /** Unsupported instruction or data found. */
    UNWIND_UNSUPPORTED,

    /** General failure. */
    UNWIND_FAILURE,

    /** Illegal instruction. */
    UNWIND_ILLEGAL_INSTR,

    /** Unwinding hit the reset vector. */
    UNWIND_RESET,

    /** Failed read for an instruction word. */
    UNWIND_IREAD_W_FAIL,

    /** Failed read for an instruction half-word. */
    UNWIND_IREAD_H_FAIL,

    /** Failed read for an instruction byte. */
    UNWIND_IREAD_B_FAIL,

    /** Failed read for a data word. */
    UNWIND_DREAD_W_FAIL,

    /** Failed read for a data half-word. */
    UNWIND_DREAD_H_FAIL,

    /** Failed read for a data byte. */
    UNWIND_DREAD_B_FAIL,

    /** Failed write for a data word. */
    UNWIND_DWRITE_W_FAIL
}
UnwResult;

/** Type for function pointer for result callback.
 * The function is passed two parameters, the first is a void * pointer,
 * and the second is the return address of the function.  The bottom bit
 * of the passed address indicates the execution mode; if it is set,
 * the execution mode at the return address is Thumb, otherwise it is
 * ARM.
 *
 * The return value of this function determines whether unwinding should
 * continue or not.  If TRUE is returned, unwinding will continue and the
 * report function maybe called again in future.  If FALSE is returned,
 * unwinding will stop with UnwindStart() returning UNWIND_TRUNCATED.
 */
typedef Boolean (*UnwindReportFunc)(void   *data,
                                    Int32   address);

/** Structure that holds memory callback function pointers.
 */
typedef struct UnwindCallbacksTag
{
    /** Report an unwind result. */
    UnwindReportFunc report;

    /** Read a 32 bit word from memory.
     * The memory address to be read is passed as \a address, and
     * \a *val is expected to be populated with the read value.
     * If the address cannot or should not be read, FALSE can be
     * returned to indicate that unwinding should stop.  If TRUE
     * is returned, \a *val is assumed to be valid and unwinding
     * will continue.
     */
    Boolean (*readW)(const Int32 address, Int32 *val);

    /** Read a 16 bit half-word from memory.
     * This function has the same usage as for readW, but is expected
     * to read only a 16 bit value.
     */
    Boolean (*readH)(const Int32 address, Int16 *val);

    /** Read a byte from memory.
     * This function has the same usage as for readW, but is expected
     * to read only an 8 bit value.
     */
    Boolean (*readB)(const Int32 address, Int8  *val);

#if defined(UNW_DEBUG)
    /** Print a formatted line for debug. */
    int (*printf)(const char *format, ...);
#endif

}
UnwindCallbacks;

/***************************************************************************
 *  Macros
 **************************************************************************/

/***************************************************************************
 *  Function Prototypes
 **************************************************************************/

/** Start unwinding the current stack.
 * This will unwind the stack starting at the PC value supplied and 
 * the stack pointer value supplied.
 */
UnwResult UnwindStart(Int32                  pcValue,
                      Int32                  spValue,
                      const UnwindCallbacks *cb,
                      void                  *data);

#endif /* UNWARMINDER_H */

/* END OF FILE */
