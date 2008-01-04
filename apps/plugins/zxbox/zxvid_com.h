#ifndef ZXVIDCOMMON_H
#define ZXVIDCOMMON_H
#include "zxconfig.h"

#ifdef USE_GREY
#include "../lib/grey.h"
#endif

#include "spscr_p.h"
#include "spmain.h"
#include "spperif.h"

#if LCD_HEIGHT >= ZX_HEIGHT && LCD_WIDTH >= ZX_WIDTH
#define WIDTH   LCD_WIDTH
#define HEIGHT  LCD_HEIGHT
#define X_OFF   0
#define Y_OFF   0
#define X_STEP  1
#define Y_STEP  1
#else
#define WIDTH      320 /* 256 */
#define HEIGHT     200 /* 192 */
#define X_OFF ( (WIDTH - ZX_WIDTH)/2)
#define Y_OFF ( (HEIGHT - ZX_HEIGHT)/2)
/* calculate distance (in source) between pixels*/
#define  X_STEP  ((ZX_WIDTH<<16) / LCD_WIDTH)
#define  Y_STEP  ((ZX_HEIGHT<<16) / LCD_HEIGHT)
#endif

extern unsigned char image_array [ HEIGHT * WIDTH ];

#endif /* ZXVIDCOMMON_H */
