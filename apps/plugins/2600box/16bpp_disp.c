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

#if LCD_WIDTH < 320
short /*fb_data*/ colors[NUMCOLS];
#else
int /*fb_data*/ colors[NUMCOLS];
#endif

int tv_depth;
int tv_bytes_pp;
/* Various variables and short functions */

/* Start of image data */
#if LCD_WIDTH < 320
short *vscreen;
#else 
BYTE *vscreen;
#endif 

#if LCD_WIDTH < 320
short screenmem [ 160*192 ];
#endif


/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;

/* Optionally set by the X debugger. */
int redraw_flag = 1;


/* Inline helper to place the tv image */
static inline void
put_image (void)
{
#if LCD_WIDTH < 320
    int  X_STEP = ((tv_width<<16) / LCD_WIDTH);
    int  Y_STEP = ((tv_height<<16) / LCD_HEIGHT);
    fb_data *frameb;
    int x=0,y=0;
    int srcx, srcy=0;     /* x / y coordinates in source image */
    short* image;
    image = &screenmem[0];
    frameb = rb->lcd_framebuffer;
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        srcx = 0;           /* reset our x counter before each row... */
        for(x = 0; x < LCD_WIDTH; x++)
        {
            *frameb = image[srcx>>16];
            srcx += X_STEP;    /* move through source image */
            frameb++;
        }
        srcy += Y_STEP;      /* move through the source image... */
        image += (srcy>>16)*tv_width;   /* and possibly to the next row. */
        srcy &= 0xffff;     /* set up the y-coordinate between 0 and 1 */
    }
#endif

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
      colors[i] = LCD_RGBPACK ( Palette_GetR(i*2) , Palette_GetG(i*2) , Palette_GetB(i*2));
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
#if LCD_WIDTH >= 320
  vwidth = LCD_WIDTH;
  vheight = LCD_HEIGHT;
#else
  vwidth = tv_width;
  vheight = tv_height;
#endif

  theight = vheight;
  
  /* Turn on the keyboard. Must be done after toplevel is created */
  init_keyboard ();

  /* Create the color map */
  create_cmap ();
#if LCD_WIDTH < 320
  tv_depth=8;
  tv_bytes_pp=tv_depth/4;
#else
  tv_depth=16;
  tv_bytes_pp=tv_depth/8;
#endif
}


/* Turns on the television. */
/* argc: argument count */
/* argv: argument text */
/* returns: 1 for success, 0 for failure */
int
tv_on (int argc, char **argv)
{
#if LCD_WIDTH >= 320
  vscreen = (BYTE*)rb -> lcd_framebuffer ;
#else
  vscreen = &screenmem[0];
#endif
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
#if LCD_WIDTH >= 320
unsigned int
#else
unsigned short
#endif
tv_color (BYTE b)
{

#if LCD_WIDTH >= 320
    return (colors[b>>1] | colors [b>>1] << 16);
#else
    return (colors[b>>1]);
#endif
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

