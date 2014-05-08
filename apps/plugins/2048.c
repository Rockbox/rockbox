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
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/highscore.h"
#define WINNING_TILE 2048
#define MAX_STARTING_TILES 2
#define HISCORES_FILE PLUGIN_GAMES_DATA_DIR "/2048.scores"
#define NUM_SCORES 5
#define RESUME_FILE PLUGIN_GAMES_DATA_DIR "/2048.resume" /* unused for now */
#define WHAT_FONT FONT_SYSFIXED
#define ANIM_SLEEPTIME 5
static int grid[4][4];
static bool merged_grid[4][4];
static int old_grid[4][4];
static int score=0;
static int max_numeral_width=-1, max_numeral_height=-1, winning_tile_length;
extern void exit(int);
/* TODO
 * Load/save
 * Add nice colors
 * Add nice animations (maybe draw each step of the slide?)
 * Sandbox mode (play after win)
 */
static struct highscore highscores[NUM_SCORES];
static int rand_range(int min, int max)
{
  return rb->rand()%(max-min+1)+min;
}
static void cleanup(void)
{
  backlight_use_settings();
}
static int rand_2_or_4(void)
{
  /* 1 in 10 chance of a four */
  if(rb->rand()%10==0)
    return 4;
  else
    return 2;
}
static void slide_internal(int startx, int starty, int stopx, int stopy, int dx, int dy, int lookx, int looky)
{
  for(int y=starty;y!=stopy;y+=dy)
    {
      for(int x=startx;x!=stopx;x+=dx)
        {
          if(grid[x+lookx][y+looky]==grid[x][y] && grid[x][y] && !merged_grid[x+lookx][y+looky] && !merged_grid[x][y]) /* Slide into */
            {
              /* Each merged tile cannot be merged again */
              merged_grid[x+lookx][y+looky]=true;
              grid[x+lookx][y+looky]=2*grid[x][y];
              score+=grid[x+lookx][y+looky];
              grid[x][y]=0;
            }
          else if(grid[x+lookx][y+looky]==0) /* Empty! */
            {
              grid[x+lookx][y+looky]=grid[x][y];
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
/* Right
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
  /* Draw the grid */
  char str[32];
  for(int y=3;y>=0;--y)
    {
      for(int x=0;x<4;++x)
        {
          if(grid[x][y])
            {
              rb->snprintf(str,32,"%d", grid[x][y]);
              rb->lcd_putsxy(winning_tile_length*x,y*max_numeral_height+max_numeral_height,str);
            }
        }
    }
  /* Now draw the score, and the game title */
  rb->snprintf(str, 32, "Score: %d", score);
  int str_width, str_height;
  rb->font_getstringsize(str, &str_width, &str_height, WHAT_FONT);
  int score_leftmost=LCD_WIDTH-str_width-1;
  /* Check if there is enough space to display "Score: ", otherwise, only display the score */
  if(score_leftmost>=0)
    rb->lcd_putsxy(score_leftmost,0,str);
  else
    rb->lcd_putsxy(score_leftmost,0,str+rb->strlen("Score: "));
  /* Reuse the same string for the title */
  
  rb->snprintf(str, 32, "%d", WINNING_TILE);
  rb->font_getstringsize(str, &str_width, &str_height, WHAT_FONT);
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
static bool check_gameover(void)
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
              return true;
            }
        }
    }
  if(!numempty)
    {
      /* No empty spaces, check for valid moves */
      /* Then, get the current score */
      int oldscore=score;
      memset(&merged_grid,0,16*sizeof(bool));
      up();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid();
          score=oldscore;
          return false;
        }
      restore_old_grid();
      memset(&merged_grid,0,16*sizeof(bool));
      down();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid();
          score=oldscore; 
          return false;
        }
      restore_old_grid();
      memset(&merged_grid,0,16*sizeof(bool));
      left();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          score=oldscore;
          return false;
        }
      restore_old_grid();
      memset(&merged_grid,0,16*sizeof(bool));
      right();
      if(memcmp(&old_grid, &grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          score=oldscore;
          return false;
        }
      score=oldscore;
      draw(); /* Shame the player :) */
      rb->splash(HZ*2, "Game Over! :(");
      return true;
    }
  return false;
}
static void load_hs(void)
{
  if(rb->file_exists(HISCORES_FILE))
    highscore_load(HISCORES_FILE, highscores, NUM_SCORES);
  else
    memset(highscores, 0, sizeof(struct highscore)*NUM_SCORES);
}
static void init_game(void)
{
  for(int y=0;y<4;++y)
    {
      for(int x=0;x<4;++x)
        grid[x][y]=0;
    }
  for(int i=0;i<MAX_STARTING_TILES;++i)
    {
      place_random();
    }
  max_numeral_width=-1;
  max_numeral_height=-1;
  score=0;
  /* Find the width of the widest character so the drawing can be nicer */
  for(char c='0';c<='9';++c)
    {
      int width=rb->font_get_width(rb->font_get(WHAT_FONT), c);
      if(width>max_numeral_width)
        max_numeral_width=width;
    }
  /* Now get the height of the font */
  rb->font_getstringsize("0123456789", NULL, &max_numeral_height,WHAT_FONT);
  /* Size of the winning tile: */
  char winning_tile_buf[32];
  rb->snprintf(winning_tile_buf,32,"%d", WINNING_TILE);
  winning_tile_length=rb->strlen(winning_tile_buf)*max_numeral_width+1;
  backlight_ignore_timeout();
  rb->lcd_clear_display();
  draw();
}
static void save_game(void)
{
  rb->remove(RESUME_FILE);
  rb->creat(RESUME_FILE, 0);
  int fd=rb->open(RESUME_FILE, O_CREAT);
  unsigned int checksum=0;
  for(int x=0;x<4;++x)
    {
      for(int y=0;y<4;++y)
        {
          rb->fdprintf(fd, "%d", grid[x][y]);
          checksum+=grid[x][y];
        }
    }
  rb->write(fd, &score, sizeof(int));
  checksum^=score;
  /*rb->write(fd, &checksum, sizeof(int));*/
  rb->close(fd);
}
static int do_2048_pause_menu(void)
{
  MENUITEM_STRINGLIST(menu,"Paused", NULL, "Resume Game", "Start New Game","High Scores","Playback Control", "Quit without Saving", "Save and Quit (buggy!)");
  bool quit=false;
  while(!quit)
    {
      switch(rb->do_menu(&menu, NULL, NULL, false))
        {
        case 0:
          draw();
          return 0;
        case 1:
          return 1;
        case 2:
          highscore_show(-1,highscores, NUM_SCORES, false);
          break;
        case 3:
          playback_control(NULL);
          break;
        case 4:
          return 2;
        case 5:
          return 3;
        }
    }
  return 0;
}
static enum plugin_status do_game(void)
{
  init_game();
  int made_move=0;
  while(1)
    {
      /* Wait for a button press */
      int button = rb->button_get(true);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(false); /* Save battery */
#endif
      made_move=0;
      memset(&merged_grid,0,16*sizeof(bool));
      memcpy(&old_grid, &grid, sizeof(int)*16);
      switch(button)
        {
        case BUTTON_UP:
          for(int i=0;i<3;++i)
            {
              up();
              rb->sleep(ANIM_SLEEPTIME);
              draw();
            }
          made_move=1;
          break;
        case BUTTON_DOWN:
          for(int i=0;i<3;++i)
            {
              down();
              rb->sleep(ANIM_SLEEPTIME);              
              draw();
            }
          made_move=1;
          break;
        case BUTTON_LEFT:
          for(int i=0;i<3;++i)
            {
              left();
              rb->sleep(ANIM_SLEEPTIME);              
              draw();
            }
          made_move=1;
          break;
        case BUTTON_RIGHT:
          for(int i=0;i<3;++i)
            {
              right();
              rb->sleep(ANIM_SLEEPTIME);              
              draw();
            }
          made_move=1;
          break;
        case BUTTON_POWER:
          switch(do_2048_pause_menu())
            {
            case 0: /* resume */
              break;
            case 1: /* new game */
              init_game();
              made_move=1;
              continue;
              break;
            case 2: /* quit without saving */
              return PLUGIN_ERROR;
            case 3: /* save and quit */
              save_game();
              return PLUGIN_ERROR;
            }
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
          if(check_gameover())
            return PLUGIN_OK;
          draw();
        }
      rb->yield();
    }
}
static enum plugin_status do_2048_menu(void)
{
  MENUITEM_STRINGLIST(menu,"2048 Menu", NULL, "Start New Game","High Scores","Playback Control", "Quit");
  bool quit=false;
  while(!quit)
    {
      switch(rb->do_menu(&menu,NULL,NULL,false))
        {
        case 0: /* Start new game */
          {
            enum plugin_status ret=do_game();
            switch(ret)
              {
              case PLUGIN_OK:
                {
                  int hs_idx=highscore_update(score,0,"",highscores,NUM_SCORES);
                  if(hs_idx>=0)
                    rb->splash(HZ*2,"High Score!");
                  highscore_show(hs_idx,highscores,NUM_SCORES,false);
                  highscore_save(HISCORES_FILE,highscores,NUM_SCORES);
                  break;
                }
              case PLUGIN_USB_CONNECTED:
                highscore_update(score,0,"",highscores,NUM_SCORES);
                highscore_save(HISCORES_FILE,highscores,NUM_SCORES);
                /* Don't bother showing the high scores... */
                return ret;
              case PLUGIN_ERROR: /* exit without menu */
                return PLUGIN_OK;
              default:
                break;
              }
            break;
          }
        case 1:
          highscore_show(-1,highscores, NUM_SCORES, false);
          break;
        case 2:
          playback_control(NULL);
          break;
        case 3:
          return PLUGIN_OK;
        default:
          break;
        }
    }
  return PLUGIN_OK;
}
enum plugin_status plugin_start(const void* param)
{
  (void)param;
  rb->srand(*(rb->current_tick));
  load_hs();
  rb->lcd_setfont(WHAT_FONT);
  enum plugin_status ret=do_2048_menu();
  highscore_save(HISCORES_FILE,highscores,NUM_SCORES);
  cleanup();
  return ret;
}
