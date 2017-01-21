#include "Font.h"

SDL_Surface *fonts;

void
initFonts(SDL_Surface * image)
{
  fonts = image;
}

int
getLetterOffset(char letter)
{
  int px;
  if(letter == '!')
    px = 36 * FONT_WIDTH;
  else if(letter >= '0' && letter <= '9')
    px = (26 + (letter - '0')) * FONT_WIDTH;
  else
    px = (int) (letter - 'a') * FONT_WIDTH;
  if(px < 0) {
    fprintf(stderr, "Can't find letter: %c\n", letter);
    fflush(stderr);
    return 0;
  }
  return px;
}

/**
   Assume letter is lower case.
 */
void
drawLetter(SDL_Surface * surface, int x, int y, char letter)
{
  SDL_Rect pos, from;

  if(letter == ' ')
    return;
  pos.x = x;
  pos.y = y;
  pos.w = FONT_WIDTH;
  pos.h = fonts->h;

  from.x = getLetterOffset(letter);
  from.y = 0;
  from.w = FONT_WIDTH - 1;
  from.h = fonts->h;
  SDL_BlitSurface(fonts, &from, surface, &pos);
}

void
drawString(SDL_Surface * surface, int x, int y, char *str)
{
  int xpos = x;
  char *p = str;
  for(p = str; *p; p++) {
    drawLetter(surface, xpos, y, *p);
    xpos += (FONT_WIDTH + FONT_SPACE);
  }
}

int
getFontPixelWidth(char *s, int start, int end)
{
  return (end - start) * (FONT_WIDTH + FONT_SPACE);
}
