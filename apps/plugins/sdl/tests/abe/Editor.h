#ifndef EDITOR_H
#define EDITOR_H

#include "Main.h"
#include "Icons.h"

#define EDIT_PANEL_HEIGHT 120

typedef struct _editpanel {
  SDL_Surface *image;
  int level;
  int image_index;
} EditPanel;

extern EditPanel edit_panel;
extern Cursor editor_cursor;

void initEditor();
void editMap();
void editorMainLoop(SDL_Event * event);
void drawEditPanel();
void editorAfterScreenFlipped();

#endif
