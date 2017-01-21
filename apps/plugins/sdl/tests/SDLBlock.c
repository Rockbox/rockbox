#include <SDL.h>

#define WIDTH LCD_WIDTH
#define HEIGHT LCD_HEIGHT
#define DEPTH LCD_DEPTH
#define BPP DEPTH/8  //bytes per pixel
#define BLOCK 4 //each block pixel is BLOCKxBLOCK ordinary pixels

int blockw, blockh;
int bytesPerPix;
SDL_Surface * screen;

void setpixel8(int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  Uint8 *pixmem8;
  Uint32 colour;  
 
   colour = SDL_MapRGB( screen->format, r, g, b );
  
   pixmem8 = (Uint8*) screen->pixels  + y + x;
   *pixmem8 = colour;
}

void setpixel16(int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  Uint16 *pixmem16;
  Uint32 colour;  
 
   colour = SDL_MapRGB( screen->format, r, g, b );
  
   pixmem16 = (Uint16*) screen->pixels  + y + x;
   *pixmem16 = colour;
}

void setpixel32(int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  Uint32 *pixmem32;
  Uint32 colour;  
 
   colour = SDL_MapRGB( screen->format, r, g, b );
  
   pixmem32 = (Uint32*) screen->pixels  + y + x;
   *pixmem32 = colour;
}

void blockPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
   int itimesw;
   int xblock=x*BLOCK;
   int yblock=y*screen->pitch/bytesPerPix*BLOCK; 

    for (int i=0; i<BLOCK; i++)
    {
         itimesw = i*screen->pitch/bytesPerPix;
         for (int j=0; j<BLOCK; j++)
	{
	   switch (bytesPerPix)
  	   {
      	      case 1:
      	      {
                       setpixel8(xblock+j, yblock+itimesw, r, g, b);
                       break;
	      }
                   case 2:
                   {
                       setpixel16(xblock+j, yblock+itimesw, r, g, b);
                       break;
                   }
                   case 4:
                   {
                       setpixel32(xblock+j, yblock+itimesw, r, g, b);
                       break;
                   }
               }
          }
    }
}

void DrawScreen(SDL_Surface* screen, int h) {
 
 int x, y;
  
  if(SDL_MUSTLOCK(screen)) {
    if(SDL_LockSurface(screen) < 0) 
      return;
  }

  for(y = 0; y < blockh; y++ ) {
      for( x = 0; x < blockw; x++ ) {
         blockPixel(x, y, (x*x)/256+3*y+h, (y*y)/256+x+h, h);
      }
  }

 if(SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }
  
  SDL_Flip(screen); 
}

void Init(void)
{
   if (SDL_Init(SDL_INIT_VIDEO) < 0 ) exit(1);
   
  if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_FULLSCREEN|SDL_SWSURFACE)))
  {
      SDL_Quit();
      exit(1);
  }

  bytesPerPix=screen->format->BytesPerPixel;
  blockw = screen->pitch/bytesPerPix/BLOCK;
  blockh = screen->h/BLOCK;
  
}

#ifndef COMBINED_SDL
#define main my_main
#else
#define main SDLBlock_main
#endif

int main(int argc, char* argv[])
{
  SDL_Event event;
  
  int keypress = 0;
  int h=0; 

  Init();  
  
  while(!keypress) {
    DrawScreen(screen,h++);
    while(SDL_PollEvent(&event)) {
      switch( event.type ) 
	{
	case SDL_QUIT:
	  keypress = 1;
	  break;
	case SDL_KEYDOWN:
	  keypress = 1;
	  break;
	}
    }
  }

 SDL_Quit();
  
  return 0;
}
