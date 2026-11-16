/***************************************************************************
 * Samsung YP-P2 ClearPad input.
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#ifndef YPP2_TOUCH_TARGET_H
#define YPP2_TOUCH_TARGET_H

#include <stdbool.h>

/* Start the worker once during button initialization. */
void ypp2_touch_init(void);

/* Read the worker's cached contact state from the periodic button tick. */
bool ypp2_touch_read(int *x, int *y);

#endif
