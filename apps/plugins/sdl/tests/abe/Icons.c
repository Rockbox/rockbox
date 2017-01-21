#include "Icons.h"

#define ICON_SIZE 80
SDL_Surface *icon_back;
int icon_selection = 0;
int icon_row;
#define MOUSE_ACCEL_X 4
#define MOUSE_ACCEL_Y 3

void
paintIcons()
{
  SDL_Rect pos;
  char s[80];

  sprintf(s, "select an icon %d", icon_selection);

  SDL_BlitSurface(icon_back, NULL, screen, NULL);
  drawString(screen, 0, 0, s);

  pos.x = (icon_selection % icon_row) * ICON_SIZE;
  pos.y = 20 + (icon_selection / icon_row) * ICON_SIZE;
  pos.w = ICON_SIZE;
  pos.h = 3;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0xa0, 0xa0, 0x00, 0x00));

  pos.x = (icon_selection % icon_row) * ICON_SIZE;
  pos.y = 20 + (icon_selection / icon_row) * ICON_SIZE + ICON_SIZE - 3;
  pos.w = ICON_SIZE;
  pos.h = 3;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0xa0, 0xa0, 0x00, 0x00));

  pos.x = (icon_selection % icon_row) * ICON_SIZE;
  pos.y = 20 + (icon_selection / icon_row) * ICON_SIZE;
  pos.w = 3;
  pos.h = ICON_SIZE;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0xa0, 0xa0, 0x00, 0x00));

  pos.x = (icon_selection % icon_row) * ICON_SIZE + ICON_SIZE - 3;
  pos.y = 20 + (icon_selection / icon_row) * ICON_SIZE;
  pos.w = 3;
  pos.h = ICON_SIZE;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0xa0, 0xa0, 0x00, 0x00));

  SDL_Flip(screen);
}

void
finishBack()
{
  int x, y, i;
  SDL_Rect pos, pos2;
  i = 0;
  for(y = 0; y < screen->h / ICON_SIZE; y++) {
    for(x = 0; x < screen->w / ICON_SIZE; x++) {
      pos.x = x * ICON_SIZE;
      pos.y = y * ICON_SIZE + 20;
      pos.w = images[i]->image->w;
      pos.h = images[i]->image->h;
      memcpy(&pos2, &pos, sizeof(SDL_Rect));
      SDL_BlitSurface(images[img_back]->image, NULL, icon_back, &pos);
      SDL_BlitSurface(images[i]->image, NULL, icon_back, &pos2);
      i++;
      if(i >= image_count)
        return;
    }
  }
}

int
selectIcon()
{
  int x, y;
  SDL_Event event;

  icon_row = screen->w / ICON_SIZE;
  createBack(&icon_back);
  finishBack();

  while(1) {
    paintIcons();

    while(SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_MOUSEBUTTONUP:
        goto escape;
      case SDL_MOUSEMOTION:
        x = event.motion.x / ICON_SIZE;
        y = (event.motion.y - 20) / ICON_SIZE;
        icon_selection = x + (y * icon_row);
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          icon_selection = -1;
          goto escape;
        case SDLK_SPACE:
        case SDLK_RETURN:
          goto escape;
        case SDLK_LEFT:
          icon_selection--;
          if(icon_selection < 0)
            icon_selection = image_count - 1;
          break;
        case SDLK_RIGHT:
          icon_selection++;
          if(icon_selection >= image_count)
            icon_selection = 0;
          break;
        case SDLK_UP:
          icon_selection -= icon_row;
          if(icon_selection < 0)
            icon_selection = image_count - 1;
          break;
        case SDLK_DOWN:
          icon_selection += icon_row;
          if(icon_selection >= image_count)
            icon_selection = 0;
          break;
        default:
          break;
        }
        break;
      default:
        break;
      }
    }
    SDL_Delay(15);
  }
escape:
  SDL_FreeSurface(icon_back);

  return (icon_selection < image_count ? icon_selection : -1);
}
