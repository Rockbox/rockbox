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

#include "keymaps.h"

#define ROCKBOXVID_DRIVER_NAME "rockbox"

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

/* Initialization/Query functions */
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

void ROCKBOX_PumpEvents(_THIS)
{
    /* poll buttons */
    static long last_keystate = 0;

    unsigned button;

    /* wolf3d code */
#if defined(BUTTON_SCROLL_FWD) && defined(BUTTON_SCROLL_BACK)
    /* check clickwheel with button_get() */
    /* strafe right */
    button = rb->button_get(false);
    static int a_release = -1, d_release = -1;

    switch(button)
    {
    case BUTTON_SCROLL_FWD | BUTTON_REPEAT:
    case BUTTON_SCROLL_FWD:
        rb_press(RBSDL_STRAFERIGHT, BUTTON_SCROLL_FWD);
        d_release = *rb->current_tick + HZ / 25;
        break;
    case BUTTON_SCROLL_BACK | BUTTON_REPEAT:
    case BUTTON_SCROLL_BACK:
        rb_press(RBSDL_STRAFELEFT, BUTTON_SCROLL_BACK);
        a_release = *rb->current_tick + HZ / 25;
        break;
    default:
        if(a_release > 0 && TIME_AFTER(*rb->current_tick, a_release))
        {
            rb_release(RBSDL_STRAFELEFT, BUTTON_SCROLL_BACK);
            a_release = -1;
        }
        if(d_release > 0 && TIME_AFTER(*rb->current_tick, d_release))
        {
            rb_release(RBSDL_STRAFERIGHT, BUTTON_SCROLL_FWD);
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
        rb_press(RBSDL_QUIT, BTN_PAUSE);
        rb_release(RBSDL_QUIT, BTN_PAUSE);
    }
#else
    /* copied from doom */
    static bool holdbutton = false;
    if (rb->button_hold() != holdbutton)
    {
        if(holdbutton==0)
        {
            rb_press(RBSDL_QUIT, BTN_PAUSE);
        }
        else
        {
            rb_release(RBSDL_QUIT, BTN_PAUSE);
        }
    }
    holdbutton=rb->button_hold();
#endif

    if(released)
    {
        if(released & BTN_FIRE)
            rb_release(RBSDL_FIRE, BTN_FIRE);
        if(released & BTN_UP)
            rb_release(RBSDL_UP, BTN_UP);
        if(released & BTN_DOWN)
            rb_release(RBSDL_DOWN, BTN_DOWN);
        if(released & BTN_LEFT)
            rb_release(RBSDL_LEFT, BTN_LEFT);
        if(released & BTN_RIGHT)
            rb_release(RBSDL_RIGHT, BTN_RIGHT);
#ifdef BTN_DOWN_LEFT
        if(released & BTN_DOWN_LEFT)
            rb_release(RBSDL_DOWN_LEFT, BTN_DOWN_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(released & BTN_DOWN_RIGHT)
            rb_release(RBSDL_DOWN_RIGHT, BTN_DOWN_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(released & BTN_UP_LEFT)
            rb_release(RBSDL_UP_LEFT, BTN_UP_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(released & BTN_UP_RIGHT)
            rb_release(RBSDL_UP_RIGHT, BTN_UP_RIGHT);
#endif
    }

    if(pressed)
    {
        if(pressed & BTN_FIRE)
            rb_press(RBSDL_FIRE, BTN_FIRE);
        if(pressed & BTN_UP)
            rb_press(RBSDL_UP, BTN_UP);
        if(pressed & BTN_DOWN)
            rb_press(RBSDL_DOWN, BTN_DOWN);
        if(pressed & BTN_LEFT)
            rb_press(RBSDL_LEFT, BTN_LEFT);
        if(pressed & BTN_RIGHT)
            rb_press(RBSDL_RIGHT, BTN_RIGHT);
#ifdef BTN_DOWN_LEFT
        if(pressed & BTN_DOWN_LEFT)
            rb_press(RBSDL_DOWN_LEFT, BTN_DOWN_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(pressed & BTN_DOWN_RIGHT)
            rb_press(RBSDL_DOWN_RIGHT, BTN_DOWN_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(pressed & BTN_UP_LEFT)
            rb_press(RBSDL_UP_LEFT, BTN_UP_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(pressed & BTN_UP_RIGHT)
            rb_press(RBSDL_UP_RIGHT, BTN_UP_RIGHT);
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

SDL_Surface *ROCKBOX_SetVideoMode(_THIS, SDL_Surface *current,
                                  int width, int height, int bpp, Uint32 flags)
{
    if ( this->hidden->buffer ) {
        SDL_free( this->hidden->buffer );
    }

    if(bpp != LCD_DEPTH)
        return NULL;

    this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));

    if ( ! this->hidden->buffer ) {
        SDL_SetError("Couldn't allocate buffer for requested mode");
        return(NULL);
    }

    /* scaling buffer, must be big enough to hold the biggest
     * possible input we can be given */
    if(width != LCD_WIDTH || height != LCD_HEIGHT)
    {
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
    current->pixels = this->hidden->buffer;

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
    for(int y = rects[i].y; y < rects[i].y + rects[i].h; ++y)
    {
        for(int x = rects[i].x; x < rects[i].x + rects[i].w; ++x)
        {
            /* swap pixels directly in the framebuffer */
            rb->lcd_framebuffer[y * LCD_WIDTH + x] = swap16(rb->lcd_framebuffer[y * LCD_WIDTH + x]);
        }
    }
}
#endif

//static fb_data tmp_fb[LCD_WIDTH * LCD_HEIGHT];

static void ROCKBOX_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    if(this->screen->pixels)
    {
        for(int i = 0; i < numrects; ++i)
        {
            /* no scaling */
            if(this->hidden->w == LCD_WIDTH && this->hidden->h == LCD_HEIGHT)
            {
                rb->lcd_bitmap_part(this->screen->pixels,
                                    rects[i].x, rects[i].y,
                                    STRIDE_MAIN(this->screen->w, this->screen->h),
                                    rects[i].x, rects[i].y,
                                    rects[i].w, rects[i].h);
#if LCD_PIXELFORMAT == RGB565SWAPPED
                flip_pixels(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
#endif
            }
            /* we must scale */
            else
            {
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
#if LCD_PIXELFORMAT == RGB565SWAPPED
                flip_pixels(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
#endif
            }
        }
        rb->lcd_update();
    }
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
    if (this->screen->pixels != NULL)
    {
        SDL_free(this->screen->pixels);
        this->screen->pixels = NULL;
    }
}
