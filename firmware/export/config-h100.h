/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128

#define CONFIG_KEYPAD IRIVER_H100_PAD

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Type of mobile power, FIXME: probably different, make new type */
#define CONFIG_BATTERY BATT_LIION2200

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

/* The start address index for ROM builds */
#define ROM_START 0x11010

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IRIVER /* port controlled */

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

#endif
