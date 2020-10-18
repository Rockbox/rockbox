/*
 * This config file is for the Sony NW-A10 series
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 104

#define MODEL_NAME   "Sony NWZ-A10 Series"

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
/* sqrt(240^2 + 320^2) / 2 = 200 */
#define LCD_DPI 200

#define NWZ_HAS_SD

#include "sonynwzlinux.h"
