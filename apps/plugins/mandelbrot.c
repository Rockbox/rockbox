/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Matthias Wientapper
 *
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SIMULATOR 
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP // this is not fun on the player

static struct plugin_api* rb;
static char buff[32];
static int lcd_aspect_ratio;
static int x_min;
static int x_max;
static int y_min;
static int y_max;
static int delta;
static int max_iter;
static unsigned char *gbuf;
static unsigned int gbuf_size = 0;

/**************** Begin grayscale framework ******************/

/* This is a generic framework to use grayscale display within
 * rockbox plugins. It obviously does not work for the player.
 *
 * If you want to use grayscale display within a plugin, copy
 * this section (up to "End grayscale framework") into your
 * source and you are able to use it. For detailed documentation
 * look at the head of each public function.
 *
 * It requires a global Rockbox api pointer in "rb" and uses 
 * timer 4 so you cannot use timer 4 for other purposes while
 * displaying grayscale.
 *
 * The framework consists of 3 sections:
 *
 * - internal core functions and definitions
 * - public core functions
 * - public optional functions
 *
 * Usually you will use functions from the latter two sections
 * in your code. You can cut out functions from the third section
 * that you do not need in order to not waste space. Don't forget
 * to cut the prototype as well.
 */

/**** internal core functions and definitions ****/

/* You do not want to touch these if you don't know exactly what
 * you're doing.
 */

#define GRAY_RUNNING          0x0001  /* grayscale overlay is running */
#define GRAY_DEFERRED_UPDATE  0x0002  /* lcd_update() requested */

typedef struct
{
    int x;
    int by;        /* 8-pixel units */
    int width;
    int height;
    int bheight;   /* 8-pixel units */
    int plane_size;
    int depth;     /* number_of_bitplanes  = (number_of_grayscales - 1) */
    int cur_plane; /* for the timer isr */
    unsigned long randmask; /* mask for random value in graypixel() */
    unsigned long flags;    /* various flags, see #defines */
    unsigned char *data;    /* pointer to start of bitplane data */
    unsigned long *bitpattern; /* pointer to start of pattern table */
} tGraybuf;

static tGraybuf *graybuf = NULL;

/** prototypes **/

void timer4_isr(void);
void graypixel(int x, int y, unsigned long pattern);
void grayinvertmasked(int x, int yb, unsigned char mask);

/** implementation **/

/* timer interrupt handler: display next bitplane */
void timer4_isr(void) /* IMIA4 */
{
    rb->lcd_blit(graybuf->data + (graybuf->plane_size * graybuf->cur_plane),
                 graybuf->x, graybuf->by, graybuf->width, graybuf->bheight,
                 graybuf->width);

    if (++graybuf->cur_plane >= graybuf->depth)
        graybuf->cur_plane = 0;

    if (graybuf->flags & GRAY_DEFERRED_UPDATE)  /* lcd_update() requested? */
    {
    	int x1 = MAX(graybuf->x, 0);
    	int x2 = MIN(graybuf->x + graybuf->width, LCD_WIDTH);
    	int y1 = MAX(graybuf->by << 3, 0);
    	int y2 = MIN((graybuf->by + graybuf->bheight) << 3, LCD_HEIGHT);

    	if(y1 > 0)  /* refresh part above overlay, full width */
    	    rb->lcd_update_rect(0, 0, LCD_WIDTH, y1);

    	if(y2 < LCD_HEIGHT) /* refresh part below overlay, full width */
    	    rb->lcd_update_rect(0, y2, LCD_WIDTH, LCD_HEIGHT - y2);

    	if(x1 > 0) /* refresh part to the left of overlay */
    	    rb->lcd_update_rect(0, y1, x1, y2 - y1);

    	if(x2 < LCD_WIDTH) /* refresh part to the right of overlay */
    	    rb->lcd_update_rect(x2, y1, LCD_WIDTH - x2, y2 - y1);

    	graybuf->flags &= ~GRAY_DEFERRED_UPDATE; /* clear request */
    }
}

/* Set a pixel to a specific bit pattern
 * This is the fundamental graphics primitive, asm optimized */
void graypixel(int x, int y, unsigned long pattern)
{
    static short random_buffer;
    register long address, mask, random;

    /* Some (pseudo-)random function must be used here to shift
     * the bit pattern randomly, otherwise you would get flicker
     * and/or moire.
     * Since rand() is relatively slow, I've implemented a simple,
     * but very fast pseudo-random generator based on linear
     * congruency in assembler. It delivers 16 pseudo-random bits
     * in each iteration.
     */

    /* simple but fast pseudo-random generator */
    asm(
        "mov.w   @%1,%0          \n"  /* load last value */
        "mov     #75,r1          \n"
        "mulu    %0,r1           \n"  /* multiply by 75 */
        "sts     macl,%0         \n"  /* get result */
        "add     #74,%0          \n"  /* add another 74 */
        "mov.w   %0,@%1          \n"  /* store new value */
        /* Since the lower bits are not very random: */
        "shlr8   %0              \n"  /* get bits 8..15 (need max. 5) */
        "and     %2,%0           \n"  /* mask out unneeded bits */
        : /* outputs */
        /* %0 */ "=&r"(random)
        : /* inputs */
        /* %1 */ "r"(&random_buffer),
        /* %2 */ "r"(graybuf->randmask)
        : /* clobbers */
        "r1","macl"
    );

    /* precalculate mask and byte address in first bitplane */
    asm(
        "mov     %3,%0           \n"  /* take y as base for address offset */
        "shlr2   %0              \n"  /* shift right by 3 (= divide by 8) */
        "shlr    %0              \n"
        "mulu    %0,%2           \n"  /* multiply with width */
        "and     #7,%3           \n"  /* get lower 3 bits of y */
        "sts     macl,%0         \n"  /* get mulu result */
        "add     %4,%0           \n"  /* add base + x to get final address */

        "mov     %3,%1           \n"  /* move lower 3 bits of y out of r0 */
        "mova    .pp_table,%3    \n"  /* get address of mask table in r0 */
        "bra     .pp_end         \n"  /* skip the table */
        "mov.b   @(%3,%1),%1     \n"  /* get entry from mask table */
        
        ".align  2               \n"
    ".pp_table:                  \n"  /* mask table */
        ".byte   0x01            \n"
        ".byte   0x02            \n"
        ".byte   0x04            \n"
        ".byte   0x08            \n"
        ".byte   0x10            \n"
        ".byte   0x20            \n"
        ".byte   0x40            \n"
        ".byte   0x80            \n"

    ".pp_end:                    \n"
        : /* outputs */
        /* %0 */ "=&r"(address),
        /* %1 */ "=&r"(mask)
        : /* inputs */
        /* %2 */ "r"(graybuf->width),
        /* %3 = r0 */ "z"(y),
        /* %4 */ "r"(graybuf->data + x)
        : /* clobbers */
        "macl"
    );

    /* the hard part: set bits in all bitplanes according to pattern */
    asm(
        "cmp/hs  %1,%5           \n"  /* random >= depth ? */
        "bf      .p_ntrim        \n"
        "sub     %1,%5           \n"  /* yes: random -= depth */
        /* it's sufficient to do this once, since the mask guarantees
         * random < 2 * depth */
    ".p_ntrim:                   \n"
    
        /* calculate some addresses */
        "mulu    %4,%1           \n"  /* end address offset */
        "not     %3,r1           \n"  /* get inverse mask (for "and") */
        "sts     macl,%1         \n"  /* result of mulu */
        "mulu    %4,%5           \n"  /* address offset of <random>'th plane */
        "add     %2,%1           \n"  /* end offset -> end address */
        "sts     macl,%5         \n"  /* result of mulu */
        "add     %2,%5           \n"  /* address of <random>'th plane */
        "bra     .p_start1       \n"
        "mov     %5,r2           \n"  /* copy address */

        /* first loop: set bits from <random>'th bitplane to last */
    ".p_loop1:                   \n"
        "mov.b   @r2,r3          \n"  /* get data byte */
        "shlr    %0              \n"  /* shift bit mask, sets t bit */
        "and     r1,r3           \n"  /* reset bit (-> "white") */
        "bf      .p_white1       \n"  /* t=0? -> "white" bit */
        "or      %3,r3           \n"  /* set bit ("black" bit) */
    ".p_white1:                  \n"
        "mov.b   r3,@r2          \n"  /* store data byte */
        "add     %4,r2           \n"  /* advance address to next bitplane */
    ".p_start1:                  \n"
        "cmp/hi  r2,%1           \n"  /* address < end address ? */
        "bt      .p_loop1        \n"
        
        "bra     .p_start2       \n"
        "nop                     \n"

        /* second loop: set bits from first to <random-1>'th bitplane
         * Bit setting works the other way round here to equalize average
         * execution times for bright and dark pixels */
    ".p_loop2:                   \n"
        "mov.b   @%2,r3          \n"  /* get data byte */
        "shlr    %0              \n"  /* shift bit mask, sets t bit */
        "or      %3,r3           \n"  /* set bit (-> "black") */
        "bt      .p_black2       \n"  /* t=1? -> "black" bit */
        "and     r1,r3           \n"  /* reset bit ("white" bit) */
    ".p_black2:                  \n"
        "mov.b   r3,@%2          \n"  /* store data byte */
        "add     %4,%2           \n"  /* advance address to next bitplane */
    ".p_start2:                  \n"
        "cmp/hi  %2,%5           \n"  /* address < <random>'th address ? */
        "bt      .p_loop2        \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(pattern),
        /* %1 */ "r"(graybuf->depth),
        /* %2 */ "r"(address),
        /* %3 */ "r"(mask),
        /* %4 */ "r"(graybuf->plane_size),
        /* %5 */ "r"(random)
        : /* clobbers */
        "r1", "r2", "r3", "macl"
    );
}

/* Invert the bits for 1-8 pixels within the buffer */
void grayinvertmasked(int x, int yb, unsigned char mask)
{
    asm(
        "mulu    %4,%5           \n"  /* width * by (offset of row) */
        "mov     #0,r1           \n"  /* current_plane = 0 */
        "sts     macl,r2         \n"  /* get mulu result */
        "add     r2,%1           \n"  /* -> address in 1st bitplane */
        
    ".i_loop:                    \n"
        "mov.b   @%1,r2          \n"  /* get data byte */
        "add     #1,r1           \n"  /* current_plane++; */
        "xor     %2,r2           \n"  /* invert bits */
        "mov.b   r2,@%1          \n"  /* store data byte */
        "add     %3,%1           \n"  /* advance address to next bitplane */
        "cmp/hi  r1,%0           \n"  /* current_plane < depth ? */
        "bt      .i_loop         \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(graybuf->depth),
        /* %1 */ "r"(graybuf->data + x),
        /* %2 */ "r"(mask),
        /* %3 */ "r"(graybuf->plane_size),
        /* %4 */ "r"(graybuf->width),
        /* %5 */ "r"(yb)
        : /* clobbers */
        "r1", "r2", "macl"
    );
}

/*** public core functions ***/

/** prototypes **/

int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth);
void gray_release_buffer(void);
void gray_position_display(int x, int by);
void gray_show_display(bool enable);

/** implementation **/

/* Prepare the grayscale display buffer
 *
 * arguments:
 *   gbuf      = pointer to the memory area to use (e.g. plugin buffer)
 *   gbuf_size = max usable size of the buffer
 *   width     = width in pixels  (1..112)
 *   bheight   = height in 8-pixel units  (1..8)
 *   depth     = desired number of grayscales - 1  (1..32)
 *
 * result:
 *   = depth  if there was enough memory
 *   < depth  if there wasn't enough memory. The number of displayable
 *            grayscales is smaller than desired, but it still works
 *   = 0      if there wasn't even enough memory for 1 bitplane (black & white)
 *
 * You can request any depth from 1 to 32, not just powers of 2. The routine
 * performs "graceful degradation" if the memory is not sufficient for the
 * desired depth. As long as there is at least enough memory for 1 bitplane,
 * it creates as many bitplanes as fit into memory, although 1 bitplane will
 * only deliver black & white display.
 *
 * The total memory needed can be calculated as follows:
 *   total_mem =
 *     sizeof(tGraymap)      (= 48 bytes currently)
 *   + sizeof(long)          (=  4 bytes)
 *   + (width * bheight + sizeof(long)) * depth
 */
int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth)
{
    int plane_size;
    int possible_depth;
    int i, j;

    if (width > LCD_WIDTH || bheight > (LCD_HEIGHT >> 3) || depth < 1)
        return 0;

    plane_size = width * bheight;
    possible_depth = (gbuf_size - sizeof(tGraybuf) - sizeof(unsigned long))
                     / (plane_size + sizeof(unsigned long));

    if (possible_depth < 1)
        return 0;

    depth = MIN(depth, 32);
    depth = MIN(depth, possible_depth);

    graybuf = (tGraybuf *) gbuf; /* global pointer to buffer structure */

    graybuf->x = 0;
    graybuf->by = 0;
    graybuf->width = width;
    graybuf->height = bheight << 3;
    graybuf->bheight = bheight;
    graybuf->plane_size = plane_size;
    graybuf->depth = depth;
    graybuf->cur_plane = 0;
    graybuf->flags = 0;
    graybuf->data = gbuf + sizeof(tGraybuf);
    graybuf->bitpattern = (unsigned long *) (graybuf->data 
                           + depth * plane_size);
    
    i = depth;
    j = 8;
    while (i != 0)
    {
    	i >>= 1;
    	j--;
    }
    graybuf->randmask = 0xFF >> j;

    /* initial state is all white */
    rb->memset(graybuf->data, 0, depth * plane_size);
    
    /* Precalculate the bit patterns for all possible pixel values */
    for (i = 0; i <= depth; i++)
    {
    	unsigned long pattern = 0;
    	int value = 0;

    	for (j = 0; j < depth; j++)
    	{
            pattern <<= 1;
            value += i;

            if (value >= depth)
                value -= depth;   /* "white" bit */
            else
                pattern |= 1;     /* "black" bit */
    	}  
        /* now the lower <depth> bits contain the pattern */

    	graybuf->bitpattern[i] = pattern;
    }
    
    return depth;
}

/* Release the grayscale display buffer
 *
 * Switches the grayscale overlay off at first if it is still running,
 * then sets the pointer to NULL.
 * DO CALL either this function or at least gray_show_display(false)
 * before you exit, otherwise nasty things may happen.
 */
void gray_release_buffer(void)
{
    gray_show_display(false);
    graybuf = NULL;
}

/* Set position of the top left corner of the grayscale overlay
 *
 * arguments:
 *   x  = left margin in pixels
 *   by = top margin in 8-pixel units
 *
 * You may set this in a way that the overlay spills across the right or
 * bottom display border. In this case it will simply be clipped by the 
 * LCD controller. You can even set negative values, this will clip at the
 * left or top border. I did not test it, but the limits may be +127 / -128
 *
 * If you use this while the grayscale overlay is running, the now-freed area
 * will be restored.
 */
void gray_position_display(int x, int by)
{
    if (graybuf == NULL)
        return;

    graybuf->x = x;
    graybuf->by = by;
    
    if (graybuf->flags & GRAY_RUNNING)
        graybuf->flags |= GRAY_DEFERRED_UPDATE;
}

/* Switch the grayscale overlay on or off
 *
 * arguments:
 *   enable = true:  the grayscale overlay is switched on if initialized
 *          = false: the grayscale overlay is switched off and the regular lcd
 *                   content is restored
 *
 * DO NOT call lcd_update() or any other api function that directly accesses
 * the lcd while the grayscale overlay is running! If you need to do
 * lcd_update() to update something outside the grayscale overlay area, use 
 * gray_deferred_update() instead.
 *
 * Other functions to avoid are:
 *   lcd_blit() (obviously), lcd_update_rect(), lcd_set_contrast(),
 *   lcd_set_invert_display(), lcd_set_flip(), lcd_roll()
 *
 * The grayscale display consumes ~50 % CPU power (for a full screen overlay,
 * less if the overlay is smaller) when switched on. You can switch the overlay
 * on and off as many times as you want.
 */
void gray_show_display(bool enable)
{
    if (graybuf == NULL)
        return;

    if (enable)
    {
        graybuf->flags |= GRAY_RUNNING;
        rb->plugin_register_timer(FREQ / 67, 1, timer4_isr);
    }
    else
    {
        rb->plugin_unregister_timer();
        graybuf->flags &= ~GRAY_RUNNING;
        rb->lcd_update(); /* restore whatever there was before */
    }
}

/*** public optional functions ***/

/* Here are the various graphics primitives. Cut out functions you do not
 * need in order to keep plugin code size down.
 */

/** prototypes **/

/* functions affecting the whole display */
void gray_clear_display(void);
//void gray_black_display(void);
//void gray_deferred_update(void);

/* scrolling functions */
//void gray_scroll_left(int count, bool black_border);
//void gray_scroll_right(int count, bool black_border);
//void gray_scroll_up8(bool black_border);
//void gray_scroll_down8(bool black_border);
//void gray_scroll_up1(bool black_border);
//void gray_scroll_down1(bool black_border);
//
/* pixel functions */
void gray_drawpixel(int x, int y, int brightness);
//void gray_invertpixel(int x, int y);

/* line functions */
//void gray_drawline(int x1, int y1, int x2, int y2, int brightness);
//void gray_invertline(int x1, int y1, int x2, int y2);

/* rectangle functions */
//void gray_drawrect(int x1, int y1, int x2, int y2, int brightness);
//void gray_fillrect(int x1, int y1, int x2, int y2, int brightness);
//void gray_invertrect(int x1, int y1, int x2, int y2);

/* bitmap functions */
//void gray_drawgraymap(unsigned char *src, int x, int y, int nx, int ny);
//void gray_drawbitmap(unsigned char *src, int x, int y, int nx, int ny,
//                     bool draw_bg, int fg_brightness, int bg_brightness);

/** implementation **/

/* Clear the grayscale display (sets all pixels to white)
 */
void gray_clear_display(void)
{
    if (graybuf == NULL)
        return;

    rb->memset(graybuf->data, 0, graybuf->depth * graybuf->plane_size);
}

/* Set the grayscale display to all black
 */
void gray_black_display(void)
{
    if (graybuf == NULL)
        return;

    rb->memset(graybuf->data, 0xFF, graybuf->depth * graybuf->plane_size);
}

/* Do a lcd_update() to show changes done by rb->lcd_xxx() functions (in areas
 * of the screen not covered by the grayscale overlay). If the grayscale 
 * overlay is running, the update will be done in the next call of the
 * interrupt routine, otherwise it will be performed right away. See also
 * comment for the gray_show_display() function.
 */
void gray_deferred_update(void)
{
    if (graybuf != NULL && (graybuf->flags & GRAY_RUNNING))
        graybuf->flags |= GRAY_DEFERRED_UPDATE;
    else
        rb->lcd_update();
}


/* Set a pixel to a specific gray value
 *
 * brightness is 0..255 (black to white) regardless of real bit depth
 */
void gray_drawpixel(int x, int y, int brightness)
{
    if (graybuf == NULL || x >= graybuf->width || y >= graybuf->height
        || brightness > 255)
        return;

    graypixel(x, y, graybuf->bitpattern[(brightness
                    * (graybuf->depth + 1)) >> 8]);
}

/* Invert a pixel
 *
 * The bit pattern for that pixel in the buffer is inverted, so white
 * becomes black, light gray becomes dark gray etc.
 */
void gray_invertpixel(int x, int y)
{
    if (graybuf == NULL || x >= graybuf->width || y >= graybuf->height)
        return;

    grayinvertmasked(x, (y >> 3), 1 << (y & 7));
}





/**************** end grayscale framework ********************/



void init_mandelbrot_set(void){
    x_min = -5<<25;  // -2.0<<26
    x_max =  1<<26;  //  1.0<<26
    y_min = -1<<26;  // -1.0<<26
    y_max =  1<<26;  //  1.0<<26
    delta = (x_max - x_min) >> 3;  // /8
    max_iter = 25;
}

void calc_mandelbrot_set(void){
    
    unsigned int start_tick;
    int n_iter;
    int x_pixel, y_pixel;
    int x, x2, y, y2, a, b;
    int x_fact, y_fact;
    int brightness;
    
    start_tick = *rb->current_tick;  
    
//    rb->lcd_clear_display();
//    rb->lcd_update();

    gray_clear_display();

    x_fact = (x_max - x_min) / LCD_WIDTH;
    y_fact = (y_max - y_min) / LCD_HEIGHT;
    
    for(y_pixel = LCD_HEIGHT-1; y_pixel>=0; y_pixel--){    
        b = (y_pixel * y_fact) + y_min;
        for (x_pixel = LCD_WIDTH-1; x_pixel>=0; x_pixel--){
            a = (x_pixel * x_fact) + x_min;
            x = 0;
            y = 0;
            n_iter = 0;
            
            while (++n_iter<=max_iter) {
                x >>= 13;
                y >>= 13;
                x2 = x * x;
                y2 = y * y;
                
                if (x2 + y2 > (4<<26)) break;
                
                y = 2 * x * y + b;
                x = x2 - y2 + a;
            }
	    
            // "coloring"
	    brightness = 0;
            if  (n_iter > max_iter){
		brightness = 0; // black
	    } else {
		brightness = 255 - (31 * (n_iter & 7));
	    }
	
	    gray_drawpixel( x_pixel, y_pixel, brightness);
	}
    }
}


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int grayscales;
    bool redraw = true;

    TEST_PLUGIN_API(api);
    rb = api;
    (void)parameter;

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the grayscale buffer:
     * 112 pixels wide, 8 rows (64 pixels) high, (try to) reserve
     * 16 bitplanes for 17 shades of gray.*/
    grayscales = gray_init_buffer(gbuf, gbuf_size, 112, 8, 16) + 1;
    if (grayscales != 17){
	rb->snprintf(buff, sizeof(buff), "%d", grayscales);
	rb->lcd_puts(0, 1, buff);
	rb->lcd_update();
	rb->sleep(HZ*2);
	return(0);
    }

    gray_show_display(true); /* switch on grayscale overlay */

    init_mandelbrot_set();
    lcd_aspect_ratio = ((LCD_WIDTH<<13) / LCD_HEIGHT)<<13;

    /* main loop */
    while (true){
        if(redraw)
            calc_mandelbrot_set();

        redraw = false;
        
        switch (rb->button_get(true)) {
        case BUTTON_OFF:
            gray_release_buffer();
            return PLUGIN_OK;

        case BUTTON_ON:
            x_min -= ((delta>>13)*(lcd_aspect_ratio>>13));
            x_max += ((delta>>13)*(lcd_aspect_ratio>>13));
            y_min -= delta;
            y_max += delta;
            delta = (x_max - x_min) >> 3;
            redraw = true;
            break;
 	        

        case BUTTON_PLAY:
            x_min += ((delta>>13)*(lcd_aspect_ratio>>13));
            x_max -= ((delta>>13)*(lcd_aspect_ratio>>13));
            y_min += delta;
            y_max -= delta;
            delta = (x_max - x_min) >> 3;
            redraw = true;
            break;
      
        case BUTTON_UP:
            y_min -= delta;
            y_max -= delta;
            redraw = true;
            break;

        case BUTTON_DOWN:
            y_min += delta;
            y_max += delta;
            redraw = true;
            break;

        case BUTTON_LEFT:
            x_min -= delta;
            x_max -= delta;
            redraw = true;
            break;

        case BUTTON_RIGHT:
            x_min += delta;
            x_max += delta;
            redraw = true;
            break;

        case BUTTON_F1:
            if (max_iter>5){
                max_iter -= 5;
                redraw = true;
            }
            break;

        case BUTTON_F2:
            if (max_iter < 195){
                max_iter += 5;
                redraw = true;
            }
            break;

        case BUTTON_F3:
            init_mandelbrot_set();
            redraw = true;
            break;

        case SYS_USB_CONNECTED:
            gray_release_buffer();
            rb->usb_screen();
            return PLUGIN_USB_CONNECTED;
        }
    }
    gray_release_buffer();
    return false;
}
#endif
#endif
