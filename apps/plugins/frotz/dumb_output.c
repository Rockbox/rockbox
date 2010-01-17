/* dumb-output.c
 * $Id: dumb-output.c,v 1.2 2002/03/26 22:52:31 feedle Exp $
 *
 * Copyright 1997,1998 Alfresco Petrofsky <alfresco@petrofsky.berkeley.edu>.
 * Any use permitted provided this notice stays intact.
 *
 * Changes for Rockbox copyright 2009 Torne Wuff
 *
 * Rockbox is not really a dumb terminal (it supports printing text wherever
 * you like) but it doesn't implement a terminal type buffer, so this is
 * close enough to be a good starting point. Keeping a copy of the graphical
 * framebuffer would be too expensive, text+attributes is much smaller.
 */

#include "dumb_frotz.h"

/* The in-memory state of the screen.  */
/* Each cell contains a style in the upper byte and a char in the lower. */
typedef unsigned short cell;
static cell screen_data[(LCD_WIDTH/SYSFONT_WIDTH) * (LCD_HEIGHT/SYSFONT_HEIGHT)];

static cell make_cell(int style, char c) {return (style << 8) | (0xff & c);}
static char cell_char(cell c) {return c & 0xff;}
static int cell_style(cell c) {return c >> 8;}

static int current_style = 0;
static int cursor_row = 0, cursor_col = 0;

static cell *dumb_row(int r) {return screen_data + r * h_screen_cols;}

int os_char_width (zchar z)
{
  (void)z;
  return 1;
}

int os_string_width (const zchar *s)
{
  int width = 0;
  zchar c;

  while ((c = *s++) != 0)
    if (c == ZC_NEW_STYLE || c == ZC_NEW_FONT)
      s++;
    else
      width += os_char_width(c);

  return width;
}

void os_set_cursor(int row, int col)
{
  cursor_row = row - 1; cursor_col = col - 1;
  if (cursor_row >= h_screen_rows)
    cursor_row = h_screen_rows - 1;
}

/* Set a cell */
static void dumb_set_cell(int row, int col, cell c)
{
  dumb_row(row)[col] = c;
}

void os_set_text_style(int x)
{
  current_style = x & REVERSE_STYLE;
}

static void dumb_display_cell(int row, int col)
{
  cell cel = dumb_row(row)[col];
  char c = cell_char(cel);
  if (!c)
      c = ' ';
  char style = cell_style(cel);
  char s[5];
  char *end = rb->utf8encode(c, s);
  *end = 0;
  if (style & REVERSE_STYLE)
    rb->lcd_set_drawmode(DRMODE_INVERSEVID);
  rb->lcd_putsxy(col * SYSFONT_WIDTH, row * SYSFONT_HEIGHT, s);
  rb->lcd_set_drawmode(DRMODE_SOLID);
}

/* put a character in the cell at the cursor and advance the cursor.  */
static void dumb_display_char(char c)
{
  dumb_set_cell(cursor_row, cursor_col, make_cell(current_style, c));
  if (++cursor_col == h_screen_cols) {
    if (cursor_row == h_screen_rows - 1)
      cursor_col--;
    else {
      cursor_row++;
      cursor_col = 0;
    }
  }
}

void os_display_char (zchar c)
{
  if (c >= ZC_LATIN1_MIN /*&& c <= ZC_LATIN1_MAX*/) {
    dumb_display_char(c);
  } else if (c >= 32 && c <= 126) {
    dumb_display_char(c);
  } else if (c == ZC_GAP) {
    dumb_display_char(' '); dumb_display_char(' ');
  } else if (c == ZC_INDENT) {
    dumb_display_char(' '); dumb_display_char(' '); dumb_display_char(' ');
  }
  return;
}


/* Haxor your boxor? */
void os_display_string (const zchar *s)
{
  zchar c;

  while ((c = *s++) != 0)
    if (c == ZC_NEW_FONT)
      s++;
    else if (c == ZC_NEW_STYLE)
      os_set_text_style(*s++);
    else {
     os_display_char (c); 
     }
}

void os_erase_area (int top, int left, int bottom, int right)
{
  int row;
  top--; left--; bottom--; right--;
  if (left == 0 && right == h_screen_cols - 1)
    rb->memset(dumb_row(top), 0, (bottom - top + 1) * h_screen_cols * sizeof(cell));
  else
    for (row = top; row <= bottom; row++)
      rb->memset(dumb_row(row) + left, 0, (right - left + 1) * sizeof(cell));
}

void os_scroll_area (int top, int left, int bottom, int right, int units)
{
  int row;
  top--; left--; bottom--; right--;
  if (units > 0) {
    for (row = top; row <= bottom - units; row++)
      rb->memcpy(dumb_row(row) + left,
              dumb_row(row + units) + left,
              (right - left + 1) * sizeof(cell));
    os_erase_area(bottom - units + 2, left + 1, bottom + 1, right + 1);
  } else if (units < 0) {
    for (row = bottom; row >= top - units; row--)
      rb->memcpy(dumb_row(row) + left,
              dumb_row(row + units) + left,
              (right - left + 1) * sizeof(cell));
    os_erase_area(top + 1, left + 1, top - units, right + 1);
  }
}

int os_font_data(int font, int *height, int *width)
{
    if (font == TEXT_FONT) {
      *height = 1; *width = 1; return 1;
    }
    return 0;
}

void os_set_colour (int x, int y) { (void)x; (void)y; }
void os_set_font (int x) { (void)x; }

/* Unconditionally show whole screen. */
void dumb_dump_screen(void)
{
  int r, c;
  rb->lcd_clear_display();
  for (r = 0; r < h_screen_height; r++)
    for (c = 0; c < h_screen_width; c++)
      dumb_display_cell(r, c);
  rb->lcd_update();
}

/* Show the current screen contents. */
void dumb_show_screen(bool show_cursor)
{
  (void)show_cursor;
  dumb_dump_screen();
}

void os_more_prompt(void)
{
  int old_row = cursor_row;
  int old_col = cursor_col;
  int old_style = current_style;
  
  current_style = REVERSE_STYLE;
  os_display_string("[MORE]");
  wait_for_key();

  cursor_row = old_row;
  cursor_col = old_col;
  current_style = old_style;
  os_erase_area(cursor_row+1, cursor_col+1, cursor_row+1, cursor_col+7);
}

void os_reset_screen(void)
{
  current_style = REVERSE_STYLE;
  os_display_string("[Press key to exit]");
  wait_for_key();
}


/* To make the common code happy */

void os_prepare_sample (int a) { (void)a; }
void os_finish_with_sample (int a) { (void)a; }
void os_start_sample (int a, int b, int c, zword d)
{
    (void)a; (void)b; (void)c; (void)d;
}
void os_stop_sample (int a) { (void)a; }

void dumb_init_output(void)
{
  if (h_version == V3) {
    h_config |= CONFIG_SPLITSCREEN;
    h_flags &= ~OLD_SOUND_FLAG;
  }

  if (h_version >= V5) {
    h_flags &= ~SOUND_FLAG;
  }

  h_screen_height = h_screen_rows;
  h_screen_width = h_screen_cols;

  h_font_width = 1; h_font_height = 1;

  os_erase_area(1, 1, h_screen_rows, h_screen_cols);
}

bool os_picture_data(int num, int *height, int *width)
{
  (void)num;
  *height = 0;
  *width = 0;
  return FALSE;
}

void os_draw_picture (int num, int row, int col)
{
  (void)num;
  (void)row;
  (void)col;
}

int os_peek_colour (void) {return BLACK_COLOUR; }
