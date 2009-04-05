#include "zxvid_com.h"

#ifdef USE_GREY

#define N0 0x00
#define N1 0xC0
#define B0 0x00
#define B1 0xFF

static unsigned char graybuffer[LCD_HEIGHT*LCD_WIDTH] IBSS_ATTR; /* off screen buffer */

static const unsigned char graylevels[16] = {
    N0, (6*N0+1*N1)/7, (5*N0+2*N1)/7, (4*N0+3*N1)/7,
    (3*N0+4*N1)/7, (2*N0+5*N1)/7, (1*N0+6*N1)/7, N1,
    B0, (6*B0+1*B1)/7, (5*B0+2*B1)/7, (4*B0+3*B1)/7,
    (3*B0+4*B1)/7, (2*B0+5*B1)/7, (1*B0+6*B1)/7, B1
};

void init_spect_scr(void)
{
    int i;
    unsigned mask = settings.invert_colors ? 0xFF : 0;

    for(i = 0; i < 16; i++) 
        sp_colors[i] = graylevels[i] ^ mask;

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
    buf_ptr = graybuffer;
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

#endif /* USE_GREY */
