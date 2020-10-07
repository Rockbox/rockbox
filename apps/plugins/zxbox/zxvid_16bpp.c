#include "zxvid_com.h"

/* screen routines for color targets */

#define N0 0x00
#define N1 0xC0
#define B0 0x00
#define B1 0xFF

#define IN0 (0xFF-N0)
#define IN1 (0xFF-N1)
#define IB0 (0xFF-B0)
#define IB1 (0xFF-B1)

static const unsigned _16bpp_colors[32] = {
    /* normal */
    LCD_RGBPACK(N0, N0, N0), LCD_RGBPACK(N0, N0, N1),
    LCD_RGBPACK(N1, N0, N0), LCD_RGBPACK(N1, N0, N1),
    LCD_RGBPACK(N0, N1, N0), LCD_RGBPACK(N0, N1, N1),
    LCD_RGBPACK(N1, N1, N0), LCD_RGBPACK(N1, N1, N1),
    LCD_RGBPACK(B0, B0, B0), LCD_RGBPACK(B0, B0, B1),
    LCD_RGBPACK(B1, B0, B0), LCD_RGBPACK(B1, B0, B1),
    LCD_RGBPACK(B0, B1, B0), LCD_RGBPACK(B0, B1, B1),
    LCD_RGBPACK(B1, B1, B0), LCD_RGBPACK(B1, B1, B1),
    /* inverted */
    LCD_RGBPACK(IN0, IN0, IN0), LCD_RGBPACK(IN0, IN0, IN1),
    LCD_RGBPACK(IN1, IN0, IN0), LCD_RGBPACK(IN1, IN0, IN1),
    LCD_RGBPACK(IN0, IN1, IN0), LCD_RGBPACK(IN0, IN1, IN1),
    LCD_RGBPACK(IN1, IN1, IN0), LCD_RGBPACK(IN1, IN1, IN1),
    LCD_RGBPACK(IB0, IB0, IB0), LCD_RGBPACK(IB0, IB0, IB1),
    LCD_RGBPACK(IB1, IB0, IB0), LCD_RGBPACK(IB1, IB0, IB1),
    LCD_RGBPACK(IB0, IB1, IB0), LCD_RGBPACK(IB0, IB1, IB1),
    LCD_RGBPACK(IB1, IB1, IB0), LCD_RGBPACK(IB1, IB1, IB1),
};

static fb_data *lcd_fb = NULL;

void init_spect_scr(void)
{
    int i;
    int offset = settings.invert_colors ? 16 : 0;

    for(i = 0; i < 16; i++) 
        sp_colors[i] = i + offset;

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
   
#if LCD_HEIGHT >= ZX_HEIGHT && LCD_WIDTH >= ZX_WIDTH
    /* 'set but not used'
    byte *scrptr;
    scrptr = (byte *) SPNM(image);
    */
    frameb = lcd_fb;
    for ( y = 0 ; y < HEIGHT*WIDTH; y++ ){
        frameb[y] = FB_SCALARPACK(_16bpp_colors[(unsigned)sp_image[y]]);
    }

#else
    int x=0;
    int srcx, srcy=0;     /* x / y coordinates in source image */
    unsigned char* image;
    image = sp_image + ( (Y_OFF)*(WIDTH) ) + X_OFF;
    frameb = lcd_fb;
    for(y = 0; y < LCD_HEIGHT; y++)
    {
        srcx = 0;           /* reset our x counter before each row... */
        for(x = 0; x < LCD_WIDTH; x++)
        {
            *frameb = FB_SCALARPACK(_16bpp_colors[image[srcx>>16]]);
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
        rb->lcd_putsxyf(0,0,"%d %%",percent);
    }
    rb -> lcd_update();
}
