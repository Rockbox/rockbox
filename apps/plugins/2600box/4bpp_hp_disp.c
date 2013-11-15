#include "config.h"
#include "version.h"
/* 
   The RockBOX display code. 
 */

/* Standard Includes */
#include "rbconfig.h"
#include <stdio.h>
#include <stdlib.h>

/* 2600 includes */
#include "types.h"
#include "vmachine.h"
#include "address.h"
#include "files.h"
#include "colours.h"
#include "keyboard.h"
#include "limiter.h"
#include "options.h"


#define Palette_GetR(x) ((BYTE) (colortable[x] >> 16))
#define Palette_GetG(x) ((BYTE) (colortable[x] >> 8))
#define Palette_GetB(x) ((BYTE) colortable[x])
#define Palette_GetY(x) (0.30 * Palette_GetR(x) + 0.59 * Palette_GetG(x) + 0.11 * Palette_GetB(x))


#define NUMCOLS 128
BYTE colors[NUMCOLS];


int tv_depth;
int tv_bytes_pp;
/* Various variables and short functions */

/* Start of image data */
BYTE *vscreen;
BYTE screen[160*228] IBSS_ATTR;

/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;

/* Optionally set by the X debugger. */
int redraw_flag = 1;



#define FB_WIDTH ((LCD_WIDTH+3)/4)
unsigned char pixmask[4] ICONST_ATTR = {
        0xC0, 0x30, 0x0C, 0x03
};

/* Inline helper to place the tv image */
static inline void
put_image (void)
{
    fb_data *frameb;
    int y=0;
    int x=0;

int X_STEP  =((tv_width<<16) / LCD_WIDTH);
int  Y_STEP  =((tv_height<<16) / LCD_HEIGHT);

	unsigned char* image;
    int srcx, srcy=0;     /* x / y coordinates in source image */
    image = &screen[0];
    unsigned mask;

    for(y = 0; y < LCD_HEIGHT; y++)
    {
        frameb = rb->lcd_framebuffer + (y) * FB_WIDTH;
        srcx = 0;           /* reset our x counter before each row... */
        for(x = 0; x < LCD_WIDTH; x++)
        {
            mask = pixmask[x & 3];
            frameb[x >> 2] = (frameb[x >> 2] & ~mask) |  ((image[(srcx>>16)]&0x3) << ((3-(x & 3 )) * 2 ));
            srcx += X_STEP;    /* move through source image */
        }
        srcy += Y_STEP;      /* move through the source image... */
        image += (srcy>>16)*tv_width;   /* and possibly to the next row. */
        srcy &= 0xffff;     /* set up the y-coordinate between 0 and 1 */
    }

    rb->lcd_update();
}

/* Create the color map of Atari colors */
/* VGA colors are only 6 bits wide */
static void
create_cmap (void)
{
  int i;
  /* Initialise parts of the colors array */
  for (i = 0; i < NUMCOLS; i++)
    {

      /* Use the color values from the color table */
	  colors[i] = LCD_BRIGHTNESS (Palette_GetY(i*2));
    }
}

/* Create the main window */
/* argc: argument count */
/* argv: argument text */
static void
create_window (int argc, char **argv)
{
  int i;

  /* Calculate the video width and height */
  vwidth = tv_width;
  
  vheight = tv_height;


  theight = vheight;
  
  /* Turn on the keyboard. Must be done after toplevel is created */
  init_keyboard ();

  /* Create the color map */
  create_cmap ();
  tv_depth=8;
  tv_bytes_pp=tv_depth/8;
}


/* Turns on the television. */
/* argc: argument count */
/* argv: argument text */
/* returns: 1 for success, 0 for failure */
int
tv_on (int argc, char **argv)
{

  vscreen = &screen[0] ;
  create_window(NULL,NULL);
  return (1);
}


/* Turn off the tv. Closes the X connection and frees the shared memory */
void
tv_off (void)
{

}


/* Translates a 2600 color value into an X11 pixel value */
/* b: byte containing 2600 colour value */
/* returns: X11 pixel value */
BYTE
tv_color (BYTE b)
{
//  return (b);
	return (colors[b]);
}

/* Displays the tv screen */
void
tv_display (void)
{
  /* Only display if the frame is a valid one. */
  if (tv_counter % base_opts.rr == 0)
    {
      put_image ();
    }
  tv_counter++;


}

/* The Event code. */
void
tv_event (void)
{
  read_keyboard ();
}

