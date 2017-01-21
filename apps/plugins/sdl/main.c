#include "plugin.h"

#include "SDL.h"
#include "SDL_video.h"

int my_main();

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    size_t sz;
    void *pluginbuf = rb->plugin_get_buffer(&sz);
    /* wipe sig */
    rb->memset(pluginbuf, 0, 4);
    init_memory_pool(sz, pluginbuf);

    rb->cpu_boost(true);

    return my_main();
}

#define WIDTH LCD_WIDTH
#define HEIGHT LCD_HEIGHT
#define DEPTH LCD_DEPTH
#define BPP (DEPTH/8)

void setpixel8(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  Uint8 *pixmem8;
  Uint32 colour;

   colour = SDL_MapRGB( screen->format, r, g, b );

   pixmem8 = (Uint8*) screen->pixels  + y + x;
   *pixmem8 = colour;
}

void setpixel16(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  Uint16 *pixmem16;
  Uint32 colour;

   colour = SDL_MapRGB( screen->format, r, g, b );

   pixmem16 = (Uint16*) screen->pixels  + y + x;
   *pixmem16 = colour;
}

void setpixel32(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  Uint32 *pixmem32;
  Uint32 colour;

   colour = SDL_MapRGB( screen->format, r, g, b );

   pixmem32 = (Uint32*) screen->pixels  + y + x;
   *pixmem32 = colour;
}

void DrawScreen(SDL_Surface* screen, int h, int bytesPerPix) {

 int x, y, ytimesw;

       if(SDL_MUSTLOCK(screen)) {
    if(SDL_LockSurface(screen) < 0)
      return;
  }

  for(y = 0; y < screen->h; y++ ) {
  ytimesw = y*screen->pitch/bytesPerPix;
  switch (bytesPerPix)
  {
      case 1:
      {
          for( x = 0; x < screen->w; x++ ) {
             setpixel8(screen, x, ytimesw, (x*x)/256+3*y+h, (y*y)/256+x+h, h);
          }
          break;
      }
      case 2:
      {
          for( x = 0; x < screen->w; x++ ) {
             setpixel16(screen, x, ytimesw, (x*x)/256+3*y+h, (y*y)/256+x+h, h);
          }
          break;
      }
      case 4:
      {
          for( x = 0; x < screen->w; x++ ) {
             setpixel32(screen, x, ytimesw, (x*x)/256+3*y+h, (y*y)/256+x+h, h);
          }
          break;
      }
   }
 }

 if(SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }

  SDL_Flip(screen);
 }


int my_main(int argc, char* argv[])
{

  SDL_Surface *screen;
  SDL_Event event;

  int keypress = 0;
  int h=0;

  if (SDL_Init(SDL_INIT_VIDEO) < 0 )
    return 1;

  if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_FULLSCREEN)))
  {
      SDL_Quit();
      return 1;
  }

  while(!keypress) {
    DrawScreen(screen,h++,screen->format->BytesPerPixel);
  }

 SDL_Quit();

  return 0;
}
