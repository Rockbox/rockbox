#ifndef FONT_H
#define FONT_H

#include "Main.h"

#define FONT_WIDTH 18
#define FONT_HEIGHT 20
#define FONT_SPACE -3

extern SDL_Surface *fonts;

void initFonts(SDL_Surface * image);
void drawLetter(SDL_Surface * surface, int x, int y, char letter);
void drawString(SDL_Surface * surface, int x, int y, char *str);
int getFontPixelWidth(char *s, int start, int end);
#endif
