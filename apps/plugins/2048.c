/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei
 *
 * Clone of 2048 by Gabriel Cirulli
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include <plugin.h>

#define WINNING_TILE 2048
#define MAX_STARTING_TILES 2
static int grid[4][4];
static bool merged_grid[4][4];
static int old_grid[4][4];
static int score=0;
extern void exit(int);
/* TODO 
 * Add nice colors
 * Add nice animations
 * Add a menu
 * Sandbox mode
 */
static int rand_range(int min, int max)
{
  return rb->rand()%(max-min+1)+min;
}
static int rand_2_or_4(void)
{
  /* 1 in 10 chance of a four */
  if(rb->rand()%10==0)
    return 4;
  else
    return 2;
}
static void slide_internal(int startx, int starty, int stopx, int stopy, int dx, int dy, int lx, int ly)
{
  for(int y=starty;y!=stopy;y+=dy)
    {
      for(int x=startx;x!=stopx;x+=dx)
        {
          if(grid[x+lx][y+ly]==grid[x][y] && grid[x][y] && !merged_grid[x+lx][y+ly] && !merged_grid[x][y]) /* Slide into */
            {
              /* Each merged tile cannot be merged again */
              merged_grid[x+lx][y+ly]=1;
              grid[x+lx][y+ly]=2*grid[x][y];
              score+=grid[x+lx][y+ly];
              grid[x][y]=0;
            }
          else if(grid[x+lx][y+ly]==0) /* Empty! */
            {
              grid[x+lx][y+ly]=grid[x][y];
              grid[x][y]=0;
            }
        }
    }
}  
/* Up
   3 
   2 ^ ^ ^ ^
   1 ^ ^ ^ ^
   0 ^ ^ ^ ^
     0 1 2 3
*/
static void up(void)
{
  slide_internal(0, 3, 
                 4, 0, 
                 1, -1, 
                 0, -1); 
}
/* Down
   3 v v v v
   2 v v v v
   1 v v v v
   0 
     0 1 2 3
*/ 
static void down(void)
{
  slide_internal(0, 2, 
                 4, -1, 
                 1, -1, 
                 0, 1); 
}
/* Left
   3   < < <
   2   < < <
   1   < < <
   0   < < <
     0 1 2 3
*/ 
static void left(void)
{
  slide_internal(1, 0, 
                 4, 4, 
                 1, 1, 
                 -1, 0); 
}
/* Left
   3 > > >  
   2 > > >  
   1 > > >  
   0 > > >  
     0 1 2 3
*/ 
static void right(void)
{
  slide_internal(0, 0, 
                 3, 4, 
                 1, 1, 
                 1, 0);
}
static void draw(void)
{
  rb->lcd_clear_display();
  for(int y=3;y>=0;--y)
    {
      for(int x=0;x<4;++x)
        {
          if(grid[x][y])
            rb->lcd_putsf(5*x,y+1,"%d",grid[x][y]);
        }
    }

  char str[32];

  rb->snprintf(str, 32, "Score: %d", score);
  int str_width, str_height;
  rb->font_getstringsize(str, &str_width, &str_height, FONT_UI);
  int score_leftmost=LCD_WIDTH-str_width-1;
  /* Check if there is enough space to display the Score:, otherwise, only display the score */
  if(score_leftmost>=0)
    rb->lcd_putsxy(score_leftmost,0,str);
  else
    rb->lcd_putsxy(score_leftmost,0,str+rb->strlen("Score: "));
  /* Reusing the same string for the title */
  
  rb->snprintf(str, 32, "%d", WINNING_TILE);
  rb->font_getstringsize(str, &str_width, &str_height, FONT_UI);
  if(str_width<score_leftmost)
    rb->lcd_putsxy(0,0,str);
  rb->lcd_update();
}
static void place_random(void)
{
  int xpos[16],ypos[16];
  int back=0;
  for(int y=0;y<4;++y)
    for(int x=0;x<4;++x)
      {
	if(!grid[x][y])
	  {
	    xpos[back]=x;
	    ypos[back]=y;
	    ++back;
	  }
      }
  if(!back)
    return;
  int idx=rand_range(0,back-1);
  grid[xpos[idx]][ypos[idx]]=rand_2_or_4();
}
static void restore_old_grid(void)
{
  memcpy(&grid, &old_grid, sizeof(int)*16);
}
static void check_gameover(void)
{
  int numempty=0;
  for(int y=0;y<4;++y)
    {
      for(int x=0;x<4;++x)
        {
          if(grid[x][y]==0)
            ++numempty;
          if(grid[x][y]==WINNING_TILE)
            {
              /* Let the user see the tile in its full glory... */
              draw();
              rb->splash(HZ*2,"You won!");
              exit(PLUGIN_OK);
            }
        }
    }
  if(!numempty)
    {
      /* No empty spaces, check for valid moves */
      up();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid();
          return;
        }
      restore_old_grid();
      down();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          return;
        }
      restore_old_grid();
      left();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          return;
        }
      restore_old_grid();
      right();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          return;
        }
      draw(); /* Shame the player :) */
      rb->splash(HZ*2, "Game over! :(");
      exit(PLUGIN_OK);
    }
}
static enum plugin_status do_game(void)
{
  for(int y=0;y<4;++y)
    {
      for(int x=0;x<4;++x)
        grid[x][y]=0;
    }
  for(int i=0;i<MAX_STARTING_TILES;++i)
    {
      grid[rand_range(0,3)][rand_range(0,3)]=rand_2_or_4();
    }
  rb->lcd_clear_display();
  draw();
  int made_move=0;
  while(1)
    {
      /* Wait for a button press */
      int button = rb->button_get(true);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(true);
#endif
      made_move=0;
      memset(&merged_grid,0,16*sizeof(bool));
      memcpy(&old_grid, &grid, sizeof(int)*16);
      switch(button)
        {
        case BUTTON_UP:
          for(int i=0;i<4;++i)
            up();
          made_move=1;
          break;
        case BUTTON_DOWN:
          for(int i=0;i<4;++i)
            down();
          made_move=1;
          break;
        case BUTTON_LEFT:
          for(int i=0;i<4;++i)
            left();
          made_move=1;
          break;
        case BUTTON_RIGHT:
          for(int i=0;i<4;++i)
            right();
          made_move=1;
          break;
        case BUTTON_POWER:
          return PLUGIN_OK;
          break;
        default:
          if (rb->default_event_handler(button) == SYS_USB_CONNECTED) 
            {
              return PLUGIN_USB_CONNECTED;
            }
          break;
        }
      if(made_move)
        {
          /* Check if we actually moved, then add random */
          if(memcmp(&old_grid, &grid, sizeof(int)*16))
            place_random();
          memcpy(&old_grid, &grid, sizeof(int)*16);
          check_gameover();
          draw();
        }
      rb->yield();
    }
}
enum plugin_status plugin_start(const void* param)
{
  (void)param;
  rb->srand(*(rb->current_tick));
  return do_game();
}
