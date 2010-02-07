#include <config.h>
#include <bmp.h>

/* To export a value for use in assembly files, define an int or unsigned here
 * named AD_<name> and include apps/core_asmdefs.h in the assembly file.
 * Identifiers without the AD_ prefix will be ignored, and can be used to
 * create instances of structs for finding offsets to individual members.
 */


/* Size of a pixel with 8-bit components. */
const int AD_pix8_size =
#ifdef HAVE_LCD_COLOR
    sizeof(struct uint8_rgb);
#else
    1;
#endif
