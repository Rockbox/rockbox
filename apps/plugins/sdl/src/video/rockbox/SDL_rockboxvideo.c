/*
  SDL - Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Sam Lantinga
  slouken@libsdl.org
*/

#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_rockboxvideo.h"

#include "plugin.h"
#include "fixedpoint.h"

#include "lib/keymaps.h"
#include "lib/pluginlib_bmp.h"

#include "keymaps_extra.h"

#define ROCKBOXVID_DRIVER_NAME "rockbox"

/* Key mapping. Receiving BTN_{x} sends RBSDL_{x} to the game. */
#define RBSDL_UP SDLK_UP
#define RBSDL_DOWN SDLK_DOWN
#define RBSDL_LEFT SDLK_LEFT
#define RBSDL_RIGHT SDLK_RIGHT
#define RBSDL_UP_LEFT SDLK_KP7
#define RBSDL_UP_RIGHT SDLK_KP9
#define RBSDL_DOWN_LEFT SDLK_KP1
#define RBSDL_DOWN_RIGHT SDLK_KP3
#define RBSDL_FIRE SDLK_RETURN
#define RBSDL_QUIT SDLK_ESCAPE
#define RBSDL_STRAFELEFT SDLK_a
#define RBSDL_STRAFERIGHT SDLK_d

/* change if needed */
#define RBSDL_EXTRA1 SDLK_1
#define RBSDL_EXTRA2 SDLK_2
#define RBSDL_EXTRA3 SDLK_3
#define RBSDL_EXTRA4 SDLK_4
#define RBSDL_EXTRA5 SDLK_5
#define RBSDL_EXTRA6 SDLK_6
#define RBSDL_EXTRA7 SDLK_7
#define RBSDL_EXTRA8 SDLK_8
#define RBSDL_EXTRA9 SDLK_9
#define RBSDL_EXTRA0 SDLK_0

/* Initialization/Query functions */
static fb_data *lcd_fb;
static int ROCKBOX_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **ROCKBOX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *ROCKBOX_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int ROCKBOX_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void ROCKBOX_VideoQuit(_THIS);

/* Hardware surface functions */
static int ROCKBOX_AllocHWSurface(_THIS, SDL_Surface *surface);
static int ROCKBOX_LockHWSurface(_THIS, SDL_Surface *surface);
static void ROCKBOX_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void ROCKBOX_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void ROCKBOX_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* ROCKBOX driver bootstrap functions */

static int ROCKBOX_Available(void)
{
    return 1;
}

static void ROCKBOX_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device->hidden->buffer);
    SDL_free(device->hidden->scale_buffer_input);
    SDL_free(device->hidden->scale_buffer_output);
    SDL_free(device->hidden);
    SDL_free(device);
}

void ROCKBOX_InitOSKeymap(_THIS) {}

static void rb_release(int sdl_sym, Uint8 scancode)
{
    LOGF("release key %d", sdl_sym);
    SDL_keysym sym;
    sym.sym = sdl_sym;
    sym.scancode = scancode;
    sym.mod = KMOD_NONE;
    SDL_PrivateKeyboard(SDL_RELEASED, &sym);
}

static void rb_press(int sdl_sym, Uint8 scancode)
{
    LOGF("pressing key %d", sdl_sym);
    SDL_keysym sym;
    sym.sym = sdl_sym;
    sym.scancode = scancode;
    sym.mod = KMOD_NONE;
    SDL_PrivateKeyboard(SDL_PRESSED, &sym);
}

typedef struct WMCursor {} WMCursor;

int ROCKBOX_ShowWMCursor(_THIS, WMcursor *cursor) { return 0; }
void ROCKBOX_WarpWMCursor(_THIS, Uint16 x, Uint16 y) {}
void ROCKBOX_MoveWMCursor(_THIS, int x, int y) {}

struct {
    unsigned up, down, left, right;

    unsigned up_left, up_right, down_left, down_right;
    unsigned fire, quit;

    unsigned strafeleft, straferight;

    unsigned extra0,
        extra1,
        extra2,
        extra3,
        extra4,
        extra5,
        extra6,
        extra7,
        extra8,
        extra9;

} rb_keymap;

void ROCKBOX_PumpEvents(_THIS)
{
    /* poll buttons */
    static long last_keystate = 0;

    unsigned button;

    /* wolf3d code */
#if defined(BUTTON_SCROLL_FWD) && defined(BUTTON_SCROLL_BACK)
    /* check clickwheel with button_get() */
    button = rb->button_get(false);
    static int a_release = -1, d_release = -1;

    switch(button)
    {
    case BUTTON_SCROLL_FWD | BUTTON_REPEAT:
    case BUTTON_SCROLL_FWD:
        rb_press(rb_keymap.straferight, BUTTON_SCROLL_FWD);
        d_release = *rb->current_tick + HZ / 25;
        break;
    case BUTTON_SCROLL_BACK | BUTTON_REPEAT:
    case BUTTON_SCROLL_BACK:
        rb_press(rb_keymap.strafeleft, BUTTON_SCROLL_BACK);
        a_release = *rb->current_tick + HZ / 25;
        break;
    default:
        if(a_release > 0 && TIME_AFTER(*rb->current_tick, a_release))
        {
            rb_release(rb_keymap.strafeleft, BUTTON_SCROLL_BACK);
            a_release = -1;
        }
        if(d_release > 0 && TIME_AFTER(*rb->current_tick, d_release))
        {
            rb_release(rb_keymap.straferight, BUTTON_SCROLL_FWD);
            d_release = -1;
        }
        break;
    }
#endif

    button = rb->button_status();

    rb->button_clear_queue();

    unsigned released = ~button & last_keystate;
    unsigned pressed = button & ~last_keystate;
    last_keystate = button;

#ifndef HAS_BUTTON_HOLD
    /* button combo for menu */
    if(button == BTN_PAUSE)
    {
        rb_press(rb_keymap.quit, BTN_PAUSE);
        rb_release(rb_keymap.quit, BTN_PAUSE);
    }
#else
    /* copied from doom */
    static bool holdbutton = false;
    if (rb->button_hold() != holdbutton)
    {
        if(holdbutton==0)
        {
            rb_press(rb_keymap.quit, BTN_PAUSE);
        }
        else
        {
            rb_release(rb_keymap.quit, BTN_PAUSE);
        }
    }
    holdbutton=rb->button_hold();
#endif

    if(released)
    {
        if(released & BTN_FIRE)
            rb_release(rb_keymap.fire, BTN_FIRE);
        if(released & BTN_UP)
            rb_release(rb_keymap.up, BTN_UP);
        if(released & BTN_DOWN)
            rb_release(rb_keymap.down, BTN_DOWN);
        if(released & BTN_LEFT)
            rb_release(rb_keymap.left, BTN_LEFT);
        if(released & BTN_RIGHT)
            rb_release(rb_keymap.right, BTN_RIGHT);
#ifdef BTN_DOWN_LEFT
        if(released & BTN_DOWN_LEFT)
            rb_release(rb_keymap.down_left, BTN_DOWN_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(released & BTN_DOWN_RIGHT)
            rb_release(rb_keymap.down_right, BTN_DOWN_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(released & BTN_UP_LEFT)
            rb_release(rb_keymap.up_left, BTN_UP_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(released & BTN_UP_RIGHT)
            rb_release(rb_keymap.up_right, BTN_UP_RIGHT);
#endif

#ifdef BTN_EXTRA0
        if(released & BTN_EXTRA0)
            rb_release(rb_keymap.extra0, BTN_EXTRA0);
#endif
#ifdef BTN_EXTRA1
        if(released & BTN_EXTRA1)
            rb_release(rb_keymap.extra1, BTN_EXTRA1);
#endif
#ifdef BTN_EXTRA2
        if(released & BTN_EXTRA2)
            rb_release(rb_keymap.extra2, BTN_EXTRA2);
#endif
#ifdef BTN_EXTRA3
        if(released & BTN_EXTRA3)
            rb_release(rb_keymap.extra3, BTN_EXTRA3);
#endif
#ifdef BTN_EXTRA4
        if(released & BTN_EXTRA4)
            rb_release(rb_keymap.extra4, BTN_EXTRA4);
#endif
#ifdef BTN_EXTRA5
        if(released & BTN_EXTRA5)
            rb_release(rb_keymap.extra5, BTN_EXTRA5);
#endif
#ifdef BTN_EXTRA6
        if(released & BTN_EXTRA6)
            rb_release(rb_keymap.extra6, BTN_EXTRA6);
#endif
#ifdef BTN_EXTRA7
        if(released & BTN_EXTRA7)
            rb_release(rb_keymap.extra7, BTN_EXTRA7);
#endif
#ifdef BTN_EXTRA8
        if(released & BTN_EXTRA8)
            rb_release(rb_keymap.extra8, BTN_EXTRA8);
#endif
#ifdef BTN_EXTRA9
        if(released & BTN_EXTRA9)
            rb_release(rb_keymap.extra9, BTN_EXTRA9);
#endif
    }

    if(pressed)
    {
        if(pressed & BTN_FIRE)
            rb_press(rb_keymap.fire, BTN_FIRE);
        if(pressed & BTN_UP)
            rb_press(rb_keymap.up, BTN_UP);
        if(pressed & BTN_DOWN)
            rb_press(rb_keymap.down, BTN_DOWN);
        if(pressed & BTN_LEFT)
            rb_press(rb_keymap.left, BTN_LEFT);
        if(pressed & BTN_RIGHT)
            rb_press(rb_keymap.right, BTN_RIGHT);
#ifdef BTN_DOWN_LEFT
        if(pressed & BTN_DOWN_LEFT)
            rb_press(rb_keymap.down_left, BTN_DOWN_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(pressed & BTN_DOWN_RIGHT)
            rb_press(rb_keymap.down_right, BTN_DOWN_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(pressed & BTN_UP_LEFT)
            rb_press(rb_keymap.up_left, BTN_UP_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(pressed & BTN_UP_RIGHT)
            rb_press(rb_keymap.up_right, BTN_UP_RIGHT);
#endif

#ifdef BTN_EXTRA0
        if(pressed & BTN_EXTRA0)
            rb_press(rb_keymap.extra0, BTN_EXTRA0);
#endif
#ifdef BTN_EXTRA1
        if(pressed & BTN_EXTRA1)
            rb_press(rb_keymap.extra1, BTN_EXTRA1);
#endif
#ifdef BTN_EXTRA2
        if(pressed & BTN_EXTRA2)
            rb_press(rb_keymap.extra2, BTN_EXTRA2);
#endif
#ifdef BTN_EXTRA3
        if(pressed & BTN_EXTRA3)
            rb_press(rb_keymap.extra3, BTN_EXTRA3);
#endif
#ifdef BTN_EXTRA4
        if(pressed & BTN_EXTRA4)
            rb_press(rb_keymap.extra4, BTN_EXTRA4);
#endif
#ifdef BTN_EXTRA5
        if(pressed & BTN_EXTRA5)
            rb_press(rb_keymap.extra5, BTN_EXTRA5);
#endif
#ifdef BTN_EXTRA6
        if(pressed & BTN_EXTRA6)
            rb_press(rb_keymap.extra6, BTN_EXTRA6);
#endif
#ifdef BTN_EXTRA7
        if(pressed & BTN_EXTRA7)
            rb_press(rb_keymap.extra7, BTN_EXTRA7);
#endif
#ifdef BTN_EXTRA8
        if(pressed & BTN_EXTRA8)
            rb_press(rb_keymap.extra8, BTN_EXTRA8);
#endif
#ifdef BTN_EXTRA9
        if(pressed & BTN_EXTRA9)
            rb_press(rb_keymap.extra9, BTN_EXTRA9);
#endif
    }
    rb->yield();
}

static SDL_VideoDevice *ROCKBOX_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
    if ( device ) {
        SDL_memset(device, 0, (sizeof *device));
        device->hidden = (struct SDL_PrivateVideoData *)
            SDL_malloc((sizeof *device->hidden));
    }
    if ( (device == NULL) || (device->hidden == NULL) ) {
        SDL_OutOfMemory();
        if ( device ) {
            SDL_free(device);
        }
        return(0);
    }
    SDL_memset(device->hidden, 0, (sizeof *device->hidden));

    /* Set the function pointers */
    device->VideoInit = ROCKBOX_VideoInit;
    device->ListModes = ROCKBOX_ListModes;
    device->SetVideoMode = ROCKBOX_SetVideoMode;
    device->CreateYUVOverlay = NULL;
    device->SetColors = ROCKBOX_SetColors;
    device->UpdateRects = ROCKBOX_UpdateRects;
    device->VideoQuit = ROCKBOX_VideoQuit;
    device->AllocHWSurface = ROCKBOX_AllocHWSurface;
    device->CheckHWBlit = NULL;
    device->FillHWRect = NULL;
    device->SetHWColorKey = NULL;
    device->SetHWAlpha = NULL;
    device->LockHWSurface = ROCKBOX_LockHWSurface;
    device->UnlockHWSurface = ROCKBOX_UnlockHWSurface;
    device->FlipHWSurface = NULL;
    device->FreeHWSurface = ROCKBOX_FreeHWSurface;
    device->SetCaption = NULL;
    device->SetIcon = NULL;
    device->IconifyWindow = NULL;
    device->GrabInput = NULL;
    device->GetWMInfo = NULL;
    device->InitOSKeymap = ROCKBOX_InitOSKeymap;
    device->PumpEvents = ROCKBOX_PumpEvents;
    device->ShowWMCursor = ROCKBOX_ShowWMCursor;
    device->WarpWMCursor = ROCKBOX_WarpWMCursor;
    device->MoveWMCursor = ROCKBOX_MoveWMCursor;

    device->free = ROCKBOX_DeleteDevice;

    return device;
}

VideoBootStrap ROCKBOX_bootstrap = {
    ROCKBOXVID_DRIVER_NAME, "SDL rockbox video driver",
    ROCKBOX_Available, ROCKBOX_CreateDevice
};

static void rb_pixelformat(SDL_PixelFormat *vformat)
{
    /* set the ideal format */
    vformat->BitsPerPixel = LCD_DEPTH;
    vformat->BytesPerPixel = LCD_DEPTH / 8;

    /* we byteswap in CopyRects() */
#if LCD_PIXELFORMAT == RGB565 || LCD_PIXELFORMAT == RGB565SWAPPED
    vformat->Rmask = 0xf800;
    vformat->Rshift = 11;
    vformat->Gmask = 0x7e0;
    vformat->Gshift = 5;
    vformat->Bmask = 0x1f;
    vformat->Bshift = 0;
#elif LCD_PIXELFORMAT == RGB888
    vformat->Rmask = 0xff0000;
    vformat->Rshift = 16;
    vformat->Gmask = 0x00ff00;
    vformat->Gshift = 8;
    vformat->Bmask = 0x0000ff;
    vformat->Bshift = 0;
#endif
}

int ROCKBOX_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    struct viewport *vp_main = *(rb->screens[SCREEN_MAIN]->current_viewport);
    lcd_fb = vp_main->buffer->fb_ptr;
    /* we change this during the SDL_SetVideoMode implementation... */
    rb_pixelformat(vformat);

    /* We're done! */
    return(0);
}

SDL_Rect **ROCKBOX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
#if 0
    static SDL_Rect rect;
    static SDL_Rect *rects[2] = { &rect, NULL };
    rect.x = rect.y = 0;
    rect.w = LCD_WIDTH;
    rect.h = LCD_HEIGHT;
    return rects;
#endif
    if(format->BitsPerPixel != LCD_DEPTH)
        return NULL;
    /* we will scale anything, as long as the format is correct */
    return (SDL_Rect**)-1;
}

static void init_keymap(void)
{
    rb_keymap.up = RBSDL_UP;
    rb_keymap.down = RBSDL_DOWN;
    rb_keymap.left = RBSDL_LEFT;
    rb_keymap.right = RBSDL_RIGHT;
    rb_keymap.up_left = RBSDL_UP_LEFT;
    rb_keymap.up_right = RBSDL_UP_RIGHT;
    rb_keymap.down_left = RBSDL_DOWN_LEFT;
    rb_keymap.down_right = RBSDL_DOWN_RIGHT;
    rb_keymap.fire = RBSDL_FIRE;
    rb_keymap.quit = RBSDL_QUIT;
    rb_keymap.strafeleft = RBSDL_STRAFELEFT;
    rb_keymap.straferight = RBSDL_STRAFERIGHT;

#ifdef BTN_EXTRA0
    rb_keymap.extra0 = RBSDL_EXTRA0;
#endif
#ifdef BTN_EXTRA1
    rb_keymap.extra1 = RBSDL_EXTRA1;
#endif
#ifdef BTN_EXTRA2
    rb_keymap.extra2 = RBSDL_EXTRA2;
#endif
#ifdef BTN_EXTRA3
    rb_keymap.extra3 = RBSDL_EXTRA3;
#endif
#ifdef BTN_EXTRA4
    rb_keymap.extra4 = RBSDL_EXTRA4;
#endif
#ifdef BTN_EXTRA5
    rb_keymap.extra5 = RBSDL_EXTRA5;
#endif
#ifdef BTN_EXTRA6
    rb_keymap.extra6 = RBSDL_EXTRA6;
#endif
#ifdef BTN_EXTRA7
    rb_keymap.extra7 = RBSDL_EXTRA7;
#endif
#ifdef BTN_EXTRA8
    rb_keymap.extra8 = RBSDL_EXTRA8;
#endif
#ifdef BTN_EXTRA9
    rb_keymap.extra9 = RBSDL_EXTRA9;
#endif
}

static void rotate_keymap(int setting)
{
    switch(setting)
    {
    case 0:
        /* normal */
        rb_keymap.up = RBSDL_UP;
        rb_keymap.down = RBSDL_DOWN;
        rb_keymap.left = RBSDL_LEFT;
        rb_keymap.right = RBSDL_RIGHT;
#ifdef RBSDL_HAVE_DIAGONAL
        rb_keymap.up_left = RBSDL_UP_LEFT;
        rb_keymap.up_right = RBSDL_UP_RIGHT;
        rb_keymap.down_left = RBSDL_DOWN_RIGHT;
        rb_keymap.down_right = RBSDL_DOWN_LEFT;
#endif
        break;
    case 1:
        /* 90deg CW */
        rb_keymap.up = RBSDL_RIGHT;
        rb_keymap.down = RBSDL_LEFT;
        rb_keymap.left = RBSDL_UP;
        rb_keymap.right = RBSDL_DOWN;
#ifdef RBSDL_HAVE_DIAGONAL
        rb_keymap.up_left = RBSDL_UP_RIGHT;
        rb_keymap.upright = RBSDL_DOWN_RIGHT;
        rb_keymap.down_left = RBSDL_UP_LEFT;
        rb_keymap.down_right = RBSDL_DOWN_LEFT;
#endif
        break;
    case 2:
        /* screen rotated 90deg CCW */
        rb_keymap.up = RBSDL_LEFT;
        rb_keymap.down = RBSDL_RIGHT;
        rb_keymap.left = RBSDL_DOWN;
        rb_keymap.right = RBSDL_UP;
#ifdef RBSDL_HAVE_DIAGONAL
        rb_keymap.up_left = RBSDL_DOWN_LEFT;
        rb_keymap.up_right = RBSDL_UP_LEFT;
        rb_keymap.down_left = RBSDL_DOWN_RIGHT;
        rb_keymap.down_right = RBSDL_DOWN_LEFT;
#endif
        break;
    }
}

SDL_Surface *ROCKBOX_SetVideoMode(_THIS, SDL_Surface *current,
                                  int width, int height, int bpp, Uint32 flags)
{
    if ( this->hidden->buffer ) {
        SDL_free( this->hidden->buffer );
    }

    if(bpp != LCD_DEPTH)
        return NULL;

    init_keymap();

    this->hidden->rotate = false;
    this->hidden->direct = false;

    /* have SDL write directly to the framebuffer */
    if(width == LCD_WIDTH && height == LCD_HEIGHT)
    {
        this->hidden->buffer = NULL;
        this->hidden->direct = true;
    }

    if(!this->hidden->direct)
    {
        this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));

        if ( ! this->hidden->buffer ) {
            SDL_SetError("Couldn't allocate buffer for requested mode");
            return(NULL);
        }
    }

    /* rotate 90deg, unless square */
    if(LCD_WIDTH != LCD_HEIGHT && width == LCD_HEIGHT && height == LCD_WIDTH)
    {
        this->hidden->rotate = true;

        /* rotate keymap, too */
        rotate_keymap(2);
    }
    else if(width != LCD_WIDTH || height != LCD_HEIGHT)
    {
        /* scaling buffer, must be big enough to hold the biggest
         * possible input we can be given */
        if(this->hidden->scale_buffer_input) SDL_free(this->hidden->scale_buffer_input);
        if(this->hidden->scale_buffer_output) SDL_free(this->hidden->scale_buffer_output);

        this->hidden->scale_buffer_input = SDL_malloc(width * height * (bpp / 8));
        this->hidden->scale_buffer_output = SDL_malloc(LCD_WIDTH * LCD_HEIGHT * (bpp / 8));
        if ( ! this->hidden->scale_buffer_input || !this->hidden->scale_buffer_output) {
            SDL_SetError("Couldn't allocate scale buffer for requested mode");
            return(NULL);
        }

        this->hidden->scale_x = fp_div(LCD_WIDTH << 16, width << 16, 16);
        this->hidden->scale_y = fp_div(LCD_HEIGHT << 16, height << 16, 16);
    }

/*      printf("Setting mode %dx%d\n", width, height); */

    if(!this->hidden->direct)
        SDL_memset(this->hidden->buffer, 0, width * height * (bpp / 8));

    /* Allocate the new pixel format for the screen */
    if ( ! SDL_ReallocFormat(current, bpp, 0, 0, 0, 0) ) {
        SDL_free(this->hidden->buffer);
        this->hidden->buffer = NULL;
        SDL_SetError("Couldn't allocate new pixel format for requested mode");
        return(NULL);
    }

    /* Set up the new mode framebuffer */
    current->flags = flags & SDL_FULLSCREEN;
    this->hidden->w = current->w = width;
    this->hidden->h = current->h = height;
    current->pitch = current->w * (bpp / 8);
    current->pixels = this->hidden->direct ? lcd_fb : this->hidden->buffer;

    /* We're done */
    return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int ROCKBOX_AllocHWSurface(_THIS, SDL_Surface *surface)
{
    return(-1);
}
static void ROCKBOX_FreeHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int ROCKBOX_LockHWSurface(_THIS, SDL_Surface *surface)
{
    return(0);
}

static void ROCKBOX_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

#if LCD_PIXELFORMAT == RGB565SWAPPED
static void flip_pixels(int x, int y, int w, int h)
{
    for(int y_0 = y; y_0 < y + h; ++y_0)
    {
        for(int x_0 = x; x_0 < x + w; ++x_0)
        {
            /* swap pixels directly in the framebuffer */
            lcd_fb[y_0 * LCD_WIDTH + x_0] = swap16(lcd_fb[y_0 * LCD_WIDTH + x_0]);
        }
    }
}
#endif

static void blit_rotated(fb_data *src, int x, int y, int w, int h)
{
    for(int y_0 = y; y_0 < y + h; ++y_0)
        for(int x_0 = x; x_0 < x + w; ++x_0)
            lcd_fb[x_0 * LCD_WIDTH + y_0] = src[(LCD_WIDTH - y_0) * LCD_HEIGHT + x_0];
}

static void ROCKBOX_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    /* Direct mode writes directly to lcd_framebuffer. Update and
     * exit! */
    if(this->hidden->direct)
    {
        rb->lcd_update();
        return;
    }

    if(this->screen->pixels)
    {
        for(int i = 0; i < numrects; ++i)
        {
            if(this->hidden->rotate)
            {
                LOGF("rotated copy");
                blit_rotated(this->screen->pixels, rects[i].x, rects[i].y, rects[i].w, rects[i].h);
            }
            /* we must scale */
            else
            {
                if(rects[i].w == this->hidden->w && rects[i].h == this->hidden->h &&
                   rects[i].x == 0 && rects[i].y == 0)
                {
                    /* special case of full screen update */
                    struct bitmap in_bmp, out_bmp;
                    in_bmp.width = rects[i].w;
                    in_bmp.height = rects[i].h;
                    in_bmp.data = this->screen->pixels;

                    out_bmp.width = LCD_WIDTH;
                    out_bmp.height = LCD_HEIGHT;
                    out_bmp.data = (char*)lcd_fb;
                    simple_resize_bitmap(&in_bmp, &out_bmp);
                }
                else
                {
                    /* This is slow. Avoid if possible. */
                    fb_data *ptr = this->hidden->scale_buffer_input;
                    /* first copy the rectangle line-by-line into our
                     * scaling buffer */
                    for(int y = rects[i].y; y < rects[i].y + rects[i].h; ++y)
                    {
                        for(int x = rects[i].x; x < rects[i].x + rects[i].w; ++x)
                        {
                            *ptr++ = ((fb_data*)this->screen->pixels)[y * this->screen->w + x];
                        }
                    }

                    /* Our scaling input buffer now has the target
                     * rectangle in it. Now we must scale it down (into
                     * the output buffer) */

                    struct bitmap in_bmp;
                    in_bmp.width = rects[i].w;
                    in_bmp.height = rects[i].h;
                    in_bmp.data = this->hidden->scale_buffer_input;

                    int out_w, out_h;
                    int out_x, out_y;
                    struct bitmap out_bmp;
                    out_w = out_bmp.width = fp_mul(rects[i].w << 16, this->hidden->scale_x, 16) >> 16;
                    out_h = out_bmp.height = fp_mul(rects[i].h << 16, this->hidden->scale_y, 16) >> 16;
                    out_bmp.data = this->hidden->scale_buffer_output;

                    simple_resize_bitmap(&in_bmp, &out_bmp);

                    out_x = fp_mul(rects[i].x << 16, this->hidden->scale_x, 16) >> 16;
                    out_y = fp_mul(rects[i].y << 16, this->hidden->scale_y, 16) >> 16;

                    /* now finally blit the output buffer to the actual
                     * framebuffer */
                    rb->lcd_bitmap(this->hidden->scale_buffer_output,
                                   out_x, out_y,
                                   out_w, out_h);
                }
            }
#if LCD_PIXELFORMAT == RGB565SWAPPED
            /* FIXME: this won't work for rotated screen or overlapping rects */
            flip_pixels(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
#endif
            /* We are lazy and do not want to figure out the new
             * rectangle coordinates. See lcd_update() below. */
            //rb->lcd_update_rect(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
        } /* for */
    } /* if */

    rb->lcd_update();
}

int ROCKBOX_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    /* do nothing of note. */
    return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void ROCKBOX_VideoQuit(_THIS)
{
    if (this->hidden->buffer != NULL)
    {
        SDL_free(this->hidden->buffer);
        this->hidden->buffer = NULL;
    }
}
