#include "zxvid_com.h"
#if !defined HAVE_LCD_COLOR && defined USE_GREY
/*
use for slightly different colors
#define N0 0x04
#define N1 0x34

#define B0 0x08
#define B1 0x3F
*/

/* these ones are the same as for color targets ... may be tweak for greyscale? */
#define N0 0x00
#define N1 0xAA

#define B0 0x55
#define B1 0xFF
static unsigned char graybuffer[LCD_HEIGHT*LCD_WIDTH] IBSS_ATTR; /* off screen buffer */

struct rgb norm_colors[COLORNUM]={
  {0,0,0},{N0,N0,N1},{N1,N0,N0},{N1,N0,N1},
  {N0,N1,N0},{N0,N1,N1},{N1,N1,N0},{N1,N1,N1},

  {0x15,0x15,0x15},{B0,B0,B1},{B1,B0,B0},{B1,B0,B1},
  {B0,B1,B0},{B0,B1,B1},{B1,B1,B0},{B1,B1,B1}
};

void init_spect_scr(void)
{
    int i;
    for(i = 0; i < 16; i++) 
        sp_colors[i] = 0.3*norm_colors[i].r + 0.59*norm_colors[i].g + 0.11*norm_colors[i].b;
    if ( settings.invert_colors ){
        int i;
        for ( i = 0 ; i < 16 ; i++ )
            sp_colors[i] = 255 - sp_colors[i];
    }

    sp_image = (char *) &image_array;
    spscr_init_mask_color();
    spscr_init_line_pointers(HEIGHT);
}


void update_screen(void)
{
    char str[80];
  
    int y=0;
    int x=0;
    unsigned char* image;
    int srcx, srcy=0;     /* x / y coordinates in source image */
    image = sp_image + ( (Y_OFF)*(WIDTH) ) + X_OFF;
    unsigned char* buf_ptr;
    buf_ptr = (unsigned char*) &graybuffer;
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        srcx = 0;           /* reset our x counter before each row... */
        for(x = 0; x < LCD_WIDTH; x++)
        {
            *buf_ptr=image[(srcx>>16)];
            srcx += X_STEP;    /* move through source image */
            buf_ptr++;
        }
        srcy += Y_STEP;      /* move through the source image... */
        image += (srcy>>16)*WIDTH;   /* and possibly to the next row. */
        srcy &= 0xffff;     /* set up the y-coordinate between 0 and 1 */
    }

#ifdef USE_BUFFERED_GREY
    grey_gray_bitmap(graybuffer, 0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif

    if ( settings.showfps ) {
        int percent=0;
        int TPF = HZ/50;/* ticks per frame */
        if ((*rb->current_tick-start_time) > TPF )
            percent = 100*video_frames/((*rb->current_tick-start_time)/TPF);
        rb->snprintf(str,sizeof(str),"%d %%",percent);
#if defined USE_BUFFERED_GREY
        grey_putsxy(0,0,str);
#else
        LOGF(str);
#endif

    }


#if defined USE_BUFFERED_GREY
    grey_update();
#else
    grey_ub_gray_bitmap(graybuffer, 0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif

}

#endif
