/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
  claim that you wrote the original software. If you use this software
  in a product, an acknowledgment in the product documentation would be
  appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_ROCKBOX

/**
 * Rockbox SDL2 video driver. Derived from offscreen driver.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_rockboxvideo.h"

#include "lib/keymaps.h"

/* Figure out what pixel format to use */
#if (LCD_PIXELFORMAT == RGB565) || (LCD_PIXELFORMAT == RGB565SWAPPED)
/* We will byteswap ourselves */
#define RBSDL_PIXELFORMAT SDL_PIXELFORMAT_RGB565
#elif (LCD_PIXELFORMAT == RGB888)
#define RBSDL_PIXELFORMAT SDL_PIXELFORMAT_RGB888
#else
#error FIXME: Unsupported pixel format
#endif

/* Key mapping. Receiving BTN_{x} sends RBSDL_{x} to the game. */
#define RBSDL_UP SDL_SCANCODE_UP
#define RBSDL_DOWN SDL_SCANCODE_DOWN
#define RBSDL_LEFT SDL_SCANCODE_LEFT
#define RBSDL_RIGHT SDL_SCANCODE_RIGHT
#define RBSDL_UP_LEFT SDL_SCANCODE_KP_7
#define RBSDL_UP_RIGHT SDL_SCANCODE_KP_9
#define RBSDL_DOWN_LEFT SDL_SCANCODE_KP_1
#define RBSDL_DOWN_RIGHT SDL_SCANCODE_KP_3
#define RBSDL_FIRE SDL_SCANCODE_RETURN
#define RBSDL_QUIT SDL_SCANCODE_ESCAPE
#define RBSDL_STRAFELEFT SDL_SCANCODE_A
#define RBSDL_STRAFERIGHT SDL_SCANCODE_D

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

typedef struct {
    bool allocated_lcdfb;
} ROCKBOX_DriverData;

static Uint32 ROCKBOX_GetGlobalMouseState(int *x, int *y)
{
    if (x) {
        *x = 0;
    }

    if (y) {
        *y = 0;
    }
    return 0;
}

static void init_keymap(void);
static void rotate_keymap(int setting);

static int ROCKBOX_VideoInit(_THIS) {
    SDL_DisplayMode mode;
    SDL_Mouse *mouse = NULL;

    mode.format = RBSDL_PIXELFORMAT;
    mode.w = LCD_WIDTH;
    mode.h = LCD_HEIGHT;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;

    SDL_AddBasicVideoDisplay(&mode);

    /* TODO: rotated mode */
    init_keymap();

    /* Init mouse */
    mouse = SDL_GetMouse();
    /* This function needs to be implemented by every driver */
    mouse->GetGlobalMouseState = ROCKBOX_GetGlobalMouseState;

    return 0;
}

static void ROCKBOX_VideoQuit(_THIS) {
}

static int ROCKBOX_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode) {
    return 0;
}

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

static void rb_press(SDL_Scancode sdl_sym) {
    SDL_SendKeyboardKey(SDL_PRESSED, sdl_sym);
}

static void rb_release(SDL_Scancode sdl_sym) {
    SDL_SendKeyboardKey(SDL_RELEASED, sdl_sym);
}

static void ROCKBOX_PumpEvents (_THIS) {
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
        rb_press(rb_keymap.straferight);
        d_release = *rb->current_tick + HZ / 25;
        break;
    case BUTTON_SCROLL_BACK | BUTTON_REPEAT:
    case BUTTON_SCROLL_BACK:
        rb_press(rb_keymap.strafeleft);
        a_release = *rb->current_tick + HZ / 25;
        break;
    default:
        if(a_release > 0 && TIME_AFTER(*rb->current_tick, a_release))
        {
            rb_release(rb_keymap.strafeleft);
            a_release = -1;
        }
        if(d_release > 0 && TIME_AFTER(*rb->current_tick, d_release))
        {
            rb_release(rb_keymap.straferight);
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
        rb_press(rb_keymap.quit);
        rb_release(rb_keymap.quit);
    }
#else
    /* copied from doom */
    static bool holdbutton = false;
    if (rb->button_hold() != holdbutton)
    {
        if(holdbutton==0)
        {
            rb_press(rb_keymap.quit);
        }
        else
        {
            rb_release(rb_keymap.quit);
        }
    }
    holdbutton=rb->button_hold();
#endif

    if(released)
    {
        if(released & BTN_FIRE)
            rb_release(rb_keymap.fire);
        if(released & BTN_UP)
            rb_release(rb_keymap.up);
        if(released & BTN_DOWN)
            rb_release(rb_keymap.down);
        if(released & BTN_LEFT)
            rb_release(rb_keymap.left);
        if(released & BTN_RIGHT)
            rb_release(rb_keymap.right);
#ifdef BTN_DOWN_LEFT
        if(released & BTN_DOWN_LEFT)
            rb_release(rb_keymap.down_left);
#endif
#ifdef BTN_DOWN_RIGHT
        if(released & BTN_DOWN_RIGHT)
            rb_release(rb_keymap.down_right);
#endif
#ifdef BTN_UP_LEFT
        if(released & BTN_UP_LEFT)
            rb_release(rb_keymap.up_left);
#endif
#ifdef BTN_UP_RIGHT
        if(released & BTN_UP_RIGHT)
            rb_release(rb_keymap.up_right);
#endif

#ifdef BTN_EXTRA0
        if(released & BTN_EXTRA0)
            rb_release(rb_keymap.extra0);
#endif
#ifdef BTN_EXTRA1
        if(released & BTN_EXTRA1)
            rb_release(rb_keymap.extra1);
#endif
#ifdef BTN_EXTRA2
        if(released & BTN_EXTRA2)
            rb_release(rb_keymap.extra2);
#endif
#ifdef BTN_EXTRA3
        if(released & BTN_EXTRA3)
            rb_release(rb_keymap.extra3);
#endif
#ifdef BTN_EXTRA4
        if(released & BTN_EXTRA4)
            rb_release(rb_keymap.extra4);
#endif
#ifdef BTN_EXTRA5
        if(released & BTN_EXTRA5)
            rb_release(rb_keymap.extra5);
#endif
#ifdef BTN_EXTRA6
        if(released & BTN_EXTRA6)
            rb_release(rb_keymap.extra6);
#endif
#ifdef BTN_EXTRA7
        if(released & BTN_EXTRA7)
            rb_release(rb_keymap.extra7);
#endif
#ifdef BTN_EXTRA8
        if(released & BTN_EXTRA8)
            rb_release(rb_keymap.extra8);
#endif
#ifdef BTN_EXTRA9
        if(released & BTN_EXTRA9)
            rb_release(rb_keymap.extra9);
#endif
    }

    if(pressed)
    {
        if(pressed & BTN_FIRE)
            rb_press(rb_keymap.fire);
        if(pressed & BTN_UP)
            rb_press(rb_keymap.up);
        if(pressed & BTN_DOWN)
            rb_press(rb_keymap.down);
        if(pressed & BTN_LEFT)
            rb_press(rb_keymap.left);
        if(pressed & BTN_RIGHT)
            rb_press(rb_keymap.right);
#ifdef BTN_DOWN_LEFT
        if(pressed & BTN_DOWN_LEFT)
            rb_press(rb_keymap.down_left);
#endif
#ifdef BTN_DOWN_RIGHT
        if(pressed & BTN_DOWN_RIGHT)
            rb_press(rb_keymap.down_right);
#endif
#ifdef BTN_UP_LEFT
        if(pressed & BTN_UP_LEFT)
            rb_press(rb_keymap.up_left);
#endif
#ifdef BTN_UP_RIGHT
        if(pressed & BTN_UP_RIGHT)
            rb_press(rb_keymap.up_right);
#endif

#ifdef BTN_EXTRA0
        if(pressed & BTN_EXTRA0)
            rb_press(rb_keymap.extra0);
#endif
#ifdef BTN_EXTRA1
        if(pressed & BTN_EXTRA1)
            rb_press(rb_keymap.extra1);
#endif
#ifdef BTN_EXTRA2
        if(pressed & BTN_EXTRA2)
            rb_press(rb_keymap.extra2);
#endif
#ifdef BTN_EXTRA3
        if(pressed & BTN_EXTRA3)
            rb_press(rb_keymap.extra3);
#endif
#ifdef BTN_EXTRA4
        if(pressed & BTN_EXTRA4)
            rb_press(rb_keymap.extra4);
#endif
#ifdef BTN_EXTRA5
        if(pressed & BTN_EXTRA5)
            rb_press(rb_keymap.extra5);
#endif
#ifdef BTN_EXTRA6
        if(pressed & BTN_EXTRA6)
            rb_press(rb_keymap.extra6);
#endif
#ifdef BTN_EXTRA7
        if(pressed & BTN_EXTRA7)
            rb_press(rb_keymap.extra7);
#endif
#ifdef BTN_EXTRA8
        if(pressed & BTN_EXTRA8)
            rb_press(rb_keymap.extra8);
#endif
#ifdef BTN_EXTRA9
        if(pressed & BTN_EXTRA9)
            rb_press(rb_keymap.extra9);
#endif
    }
    rb->yield();
}

#define ROCKBOX_FB "_ROCKBOX_fb"

static int ROCKBOX_CreateWindowFramebuffer (_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch) {
    int w, h;

    SDL_GetWindowSize(window, &w, &h);

    ROCKBOX_DriverData *data = _this->driverdata;

    /* Free old surface, if any */
    void *ptr = SDL_GetWindowData(window, ROCKBOX_FB);
    if(ptr) {
        if(ptr == rb->lcd_framebuffer) {
            data->allocated_lcdfb = false; // "free" LCD framebuffer
        } else {
            SDL_free(ptr);
        }
    }

    if(!data->allocated_lcdfb && w == LCD_WIDTH && h == LCD_HEIGHT) {
        *pixels = ptr = rb->lcd_framebuffer;
        *pitch = LCD_WIDTH * (LCD_DEPTH / 8);
        LOGF("Allocating framebuffer");
    } else {
        int bpp;
        Uint32 Rmask, Gmask, Bmask, Amask;

        SDL_PixelFormatEnumToMasks(RBSDL_PIXELFORMAT, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
        SDL_Surface *surface = ptr = SDL_CreateRGBSurface(0, w, h, bpp, Rmask, Gmask, Bmask, Amask);

        *pixels = surface->pixels;
        *pitch = surface->pitch;
    }

    *format = RBSDL_PIXELFORMAT;

    if(!ptr) {
        return -1;
    }

    SDL_SetWindowData(window, ROCKBOX_FB, ptr);

    return 0;
}

static int ROCKBOX_UpdateWindowFramebuffer (_THIS, SDL_Window * window, const SDL_Rect * rects, int numrects) {
    void *ptr = SDL_GetWindowData(window, ROCKBOX_FB);

    if(ptr == rb->lcd_framebuffer) {
        rb->lcd_update();
    }
    else {
        LOGF("WARNING: secondary framebuffers ignored");
        return -1;
    }

    return 0;
}

static void ROCKBOX_DestroyWindowFramebuffer (_THIS, SDL_Window * window) {
}

static void ROCKBOX_DeleteDevice(_THIS) {

}

static int ROCKBOX_CreateWindow (_THIS, SDL_Window * window) {
    return 0;
}

static void ROCKBOX_DestroyWindow (_THIS, SDL_Window * window) {
    return;
}

static SDL_VideoDevice *ROCKBOX_CreateDevice(int devidx) {
    SDL_VideoDevice *device;
    ROCKBOX_DriverData *data;

    device = (SDL_VideoDevice*) SDL_calloc(1, sizeof(*device));
    if(!device) {
        SDL_OutOfMemory();
        return NULL;
    }

    device->VideoInit = ROCKBOX_VideoInit;
    device->VideoQuit = ROCKBOX_VideoQuit;
    device->SetDisplayMode = ROCKBOX_SetDisplayMode;
    device->PumpEvents = ROCKBOX_PumpEvents;
    device->CreateWindowFramebuffer = ROCKBOX_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = ROCKBOX_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = ROCKBOX_DestroyWindowFramebuffer;
    device->free = ROCKBOX_DeleteDevice;

    device->CreateSDLWindow = ROCKBOX_CreateWindow;
    device->DestroyWindow = ROCKBOX_DestroyWindow;

    device->driverdata = data = SDL_calloc(1, sizeof(ROCKBOX_DriverData));

    if(!data) {
        SDL_OutOfMemory();
        return NULL;
    }

    data->allocated_lcdfb = false;

    return device;
}

static int ROCKBOX_Available(void) {
    return 1;
}

VideoBootStrap ROCKBOX_bootstrap = {
    "rockbox", "SDL rockbox video driver",
    ROCKBOX_Available, ROCKBOX_CreateDevice
};

#endif
