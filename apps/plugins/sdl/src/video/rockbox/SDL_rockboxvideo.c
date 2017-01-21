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

/* Rockbox SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "ROCKBOX" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_rockboxvideo.h"

#include "plugin.h"

#define ROCKBOXVID_DRIVER_NAME "rockbox"

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

typedef struct WMCursor {} WMCursor;

void ROCKBOX_ShowWMCursor(_THIS, WMCursor* cursor) {}
void ROCKBOX_WarpWMCursor(_THIS, Uint16 x, Uint16 y) {}
void ROCKBOX_MoveWMCursor(_THIS, int x, int y) {}

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
        device->PumpEvents = NULL;
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


int ROCKBOX_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
        /*
        fprintf(stderr, "WARNING: You are using the SDL rockbox video driver!\n");
        */

        /* Determine the screen depth (use default 8-bit depth) */
        /* we change this during the SDL_SetVideoMode implementation... */
        vformat->BitsPerPixel = LCD_DEPTH;
        vformat->BytesPerPixel = LCD_DEPTH / 8;

        /* We're done! */
        return(0);
}

SDL_Rect **ROCKBOX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    static SDL_Rect rect;
    static SDL_Rect *rects[2] = { &rect, NULL };
    if(format->BitsPerPixel != LCD_DEPTH)
        return NULL;
    rect.x = rect.y = 0;
    rect.w = LCD_WIDTH;
    rect.h = LCD_HEIGHT;
    return rects;
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

static void ROCKBOX_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    if(this->screen->pixels)
    {
        for(int i = 0; i < numrects; ++i)
        {
            //rb->splashf(HZ, "rect %d: %dx%d, (%d, %d)", i, rects[i].w, rects[i].h, rects[i].x, rects[i].y);
            rb->lcd_bitmap_part(this->screen->pixels,
                                rects[i].x, rects[i].y,
                                this->screen->pitch / 2,
                                rects[i].x, rects[i].y,
                                rects[i].w, rects[i].h);
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
