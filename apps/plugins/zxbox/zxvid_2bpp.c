#include "zxvid_com.h"

#ifndef USE_GREY
/* screen routines for greyscale targets not using greyscale lib */

#if LCD_PIXELFORMAT == HORIZONTAL_PACKING 
#define FB_WIDTH ((LCD_WIDTH+3)/4)
fb_data pixmask[4] ICONST_ATTR = {
    0xC0, 0x30, 0x0C, 0x03
};
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
fb_data pixmask[4] ICONST_ATTR = {
    0x03, 0x0C, 0x30, 0xC0
};
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
fb_data pixmask[8] ICONST_ATTR = {
    0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080
};
fb_data pixval[4] ICONST_ATTR = {
    0x0000, 0x0001, 0x0100, 0x0101
};
#endif

static const unsigned char graylevels[16] = {
    0, 1, 1, 1, 2, 2, 3, 3,
    0, 1, 1, 1, 2, 2, 3, 3
};

static fb_data *lcd_fb = NULL;

void init_spect_scr(void)
{
    int i;
    unsigned mask = settings.invert_colors ? 0 : 3;
    
    for(i = 0; i < 16; i++)
        sp_colors[i] = graylevels[i] ^ mask;

    sp_image = (char *) &image_array;
    spscr_init_mask_color();
    spscr_init_line_pointers(HEIGHT);

}
void update_screen(void)
{
    if (!lcd_fb)
    {
        struct viewport *vp_main = rb->lcd_set_viewport(NULL);
        lcd_fb = vp_main->buffer->fb_ptr;
    }
    fb_data *frameb;
    int y=0;
    int x=0;
    unsigned char* image;
    int srcx, srcy=0;     /* x / y coordinates in source image */
    image = sp_image + ( (Y_OFF)*(WIDTH) ) + X_OFF;
    unsigned mask;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        frameb = lcd_fb + (y) * FB_WIDTH;
        srcx = 0;           /* reset our x counter before each row... */
        for(x = 0; x < LCD_WIDTH; x++)
        {
            mask = ~pixmask[x & 3];
            frameb[x >> 2] = (frameb[x >> 2] & mask) |  ((image[(srcx>>16)]&0x3) << ((3-(x & 3 )) * 2 ));
            srcx += X_STEP;    /* move through source image */
        }
        srcy += Y_STEP;      /* move through the source image... */
        image += (srcy>>16)*WIDTH;   /* and possibly to the next row. */
        srcy &= 0xffff;     /* set up the y-coordinate between 0 and 1 */
    }
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
    int shift;
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        frameb = lcd_fb + (y/4) * LCD_WIDTH;
        srcx = 0;           /* reset our x counter before each row... */
        shift = ((y & 3 ) * 2 );
        mask = ~pixmask[y & 3];
        for(x = 0; x < LCD_WIDTH; x++)
        {
            frameb[x] = (frameb[x] & mask) |  ((image[(srcx>>16)]&0x3) << shift );
            srcx += X_STEP;    /* move through source image */
        }
        srcy += Y_STEP;      /* move through the source image... */
        image += (srcy>>16)*WIDTH;   /* and possibly to the next row. */
        srcy &= 0xffff;     /* set up the y-coordinate between 0 and 1 */
    }
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
    int shift;
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        frameb = lcd_fb + (y/8) * LCD_WIDTH;
        srcx = 0;           /* reset our x counter before each row... */
        shift = (y & 7);
        mask = ~pixmask[y & 7];
        for(x = 0; x < LCD_WIDTH; x++)
        {
            frameb[x] = (frameb[x] & mask) |  (pixval[image[(srcx>>16)]&0x3] << shift );
            srcx += X_STEP;    /* move through source image */
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
        rb->lcd_putsxyf(0,0,"%d %%",percent);
    }


    rb -> lcd_update();
}

#endif /* !USE_GREY */
