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
 * File Description: Unwinder client that reads local memory.
 *   This client reads from local memory and is designed to run on target
 *   along with the unwinder.  Memory read requests are implemented by
 *   casting a point to read the memory directly, although checks for
 *   alignment should probably also be made if this is to be used in
 *   production code, as otherwise the ARM may return the memory in a
 *   rotated/rolled format, or the MMU may generate an alignment exception
 *   if present and so configured.
 **************************************************************************/

/***************************************************************************
 * Includes
 ***************************************************************************/

#include "backtrace.h"
#include "safe_read.h"

/***************************************************************************
 * Prototypes
 ***************************************************************************/

static Boolean CliReport(void *data, Int32 address);
static Boolean CliReadW(Int32 a, Int32 *v);
static Boolean CliReadH(Int32 a, Int16 *v);
static Boolean CliReadB(Int32 a, Int8  *v);

/***************************************************************************
 * Variables
 ***************************************************************************/

/* Table of function pointers for passing to the unwinder */
const UnwindCallbacks cliCallbacks =
    {
        CliReport,
        CliReadW,
        CliReadH,
        CliReadB
#if defined(UNW_DEBUG)
       ,printf
#endif
    };


/***************************************************************************
 * Callbacks
 ***************************************************************************/

/***************************************************************************
 *
 * Function:     CliReport
 *
 * Parameters:   data    - Pointer to data passed to UnwindStart()
 *               address - The return address of a stack frame.
 *
 * Returns:      TRUE if unwinding should continue, otherwise FALSE to
 *                 indicate that unwinding should stop.
 *
 * Description:  This function is called from the unwinder each time a stack
 *                 frame has been unwound.  The LSB of address indicates if
 *                 the processor is in ARM mode (LSB clear) or Thumb (LSB
 *                 set).
 *
 ***************************************************************************/
static Boolean CliReport(void *data, Int32 address)
{
    /* CliStack *s = (CliStack *)data; */
    unsigned *line = (unsigned *)data;


    lcd_putsf(0, (*line)++, "    %c: %08x",
              (address & 0x1) ? 'T' : 'A',
              address & (~0x1));
    lcd_update();

    return TRUE;
}

static Boolean CliReadW(const Int32 a, Int32 *v)
{
    return safe_read32((uint32_t *)a, (uint32_t *)v);
}

static Boolean CliReadH(const Int32 a, Int16 *v)
{
    return safe_read16((void *)a, (uint16_t *)v);
}

static Boolean CliReadB(const Int32 a, Int8 *v)
{
    return safe_read8((void *)a, (uint8_t *)v);
}

Boolean CliInvalidateW(const Int32 a)
{
    *(Int32 *)a = 0xdeadbeef;
    return TRUE;
}

void backtrace(int pcAddr, int spAddr, unsigned *line)
{
    UnwResult r;

    lcd_putsf(0, (*line)++, "bt pc: %08x, sp: %08x", pcAddr, spAddr);
    lcd_update();

    r = UnwindStart(pcAddr, spAddr, &cliCallbacks, (void *)line);

    lcd_puts(0, (*line)++, "bt end");
    lcd_update();
}

/* END OF FILE */
