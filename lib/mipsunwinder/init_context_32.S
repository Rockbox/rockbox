#include "mips.h"

    .text
    .global mips_bt_start

mips_bt_start:
    addiu   v0, ra, -8
    sw      v0, 0(a0)       /* ctx->pc = ra - 8 */
    sw      sp, 4(a0)       /* ctx->sp = sp */
    sw      zero, 8(a0)     /* ctx->depth = 0 */
    sw      zero, 12(a0)    /* ctx->valid = 0 */
    jr      ra
