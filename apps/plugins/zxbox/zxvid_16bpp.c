#include "zxvid_com.h"

#if LCD_DEPTH > 4
/* screen routines for color targets */

/*
use for slightly different colors
#define N0 0x04
#define N1 0x34

#define B0 0x08
#define B1 0x3F
*/

#define N0 0x00
#define N1 0xC0

#define B0 0x00
#define B1 0xFF

struct rgb norm_colors[COLORNUM]={
  {0,0,0},{N0,N0,N1},{N1,N0,N0},{N1,N0,N1},
  {N0,N1,N0},{N0,N1,N1},{N1,N1,N0},{N1,N1,N1},

  {0,0,0},{B0,B0,B1},{B1,B0,B0},{B1,B0,B1},
  {B0,B1,B0},{B0,B1,B1},{B1,B1,B0},{B1,B1,B1}
};


/* since emulator uses array of bytes for screen representation
 * short 16b colors won't fit there */
short _16bpp_colors[16] IBSS_ATTR; 

void init_spect_scr(void)
{
    int i;

    for(i = 0; i < 16; i++) 
        sp_colors[i] = i;
    for(i = 0; i < 16; i++) 
        _16bpp_colors[i] = LCD_RGBPACK(norm_colors[i].r,norm_colors[i].g,norm_colors[i].b);
    if ( settings.invert_colors ){
        for ( i = 0 ; i < 16 ; i++ )
            _16bpp_colors[i] = 0xFFFFFF - _16bpp_colors[i];
    }
    sp_image = (char *) &image_array;
    spscr_init_mask_color();
    spscr_init_line_pointers(HEIGHT);

}

void update_screen(void)
{
    char str[80];
    fb_data *frameb;
  
    int y=0;
   
#if LCD_HEIGHT >= ZX_HEIGHT && LCD_WIDTH >= ZX_WIDTH
    byte *scrptr;
    scrptr = (byte *) SPNM(image);
    frameb = rb->lcd_framebuffer;
    for ( y = 0 ; y < HEIGHT*WIDTH; y++ ){
        frameb[y] = _16bpp_colors[(unsigned)sp_image[y]];
    }

#else
    int x=0;
    int srcx, srcy=0;     /* x / y coordinates in source image */
    unsigned char* image;
    image = sp_image + ( (Y_OFF)*(WIDTH) ) + X_OFF;
    frameb = rb->lcd_framebuffer;
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        srcx = 0;           /* reset our x counter before each row... */
        for(x = 0; x < LCD_WIDTH; x++)
        {
            *frameb = _16bpp_colors[image[srcx>>16]];
            srcx += X_STEP;    /* move through source image */
            frameb++;
        }
        srcy += Y_STEP;      /* move through the source image... */
        image += (srcy>>16)*WIDTH;   /* and possibly to the next row. */
        srcy &= 0xffff;     /* set up the y-coordinate between 0 and 1 */
    }

#endif
    if ( settings.showfps ) {
        int percent=0;
        int TPF = HZ/50;/* ticks per frame */
        if ((*rb->current_tick-start_time) > TPF )
            percent = 100*video_frames/((*rb->current_tick-start_time)/TPF);
        rb->snprintf(str,sizeof(str),"%d %%",percent);
        rb->lcd_putsxy(0,0,str);
    }
    rb -> lcd_update();
}


#endif /* HAVE_LCD_COLOR */
