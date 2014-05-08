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
#include "lib/display_text.h"
#define WINNING_TILE 2048
#define MAX_STARTING_TILES 2
#define HISCORES_FILE PLUGIN_GAMES_DATA_DIR "/2048.scores"
#define NUM_SCORES 5
#define RESUME_FILE PLUGIN_GAMES_DATA_DIR "/2048.resume" /* unused for now */
#define WHAT_FONT FONT_SYSFIXED
#define ANIM_SLEEPTIME 5
/* What is needed to save/load a game */
struct game_ctx_t {
  int grid[4][4];
  int score;
};
static struct game_ctx_t ctx_data;
/* use a pointer to make save/load easier */
static struct game_ctx_t *ctx=&ctx_data;
/* temporary data */
static bool merged_grid[4][4];
static int old_grid[4][4];
static int max_numeral_width=-1, max_numeral_height=-1, winning_tile_length;
static bool loaded=false;
extern void exit(int);
/* TODO
 * Load/save
 * Add nice colors
 * Add nice animations (maybe draw each step of the slide?)
 * Sandbox mode (play after win)
 */
static struct highscore highscores[NUM_SCORES];
static inline int rand_range(int min, int max)
{
  return rb->rand()%(max-min+1)+min;
}
static void cleanup(void)
{
  backlight_use_settings();
}
static inline int rand_2_or_4(void)
{
  /* 1 in 10 chance of a four */
  if(rb->rand()%10==0)
    return 4;
  else
    return 2;
}
static bool do_help(void)
{
  char* help_text[]= {"2048", "", "Aim", 
                      "", "Join", "the", "numbers", "to", "get", "to", "the", "2048", "tile!", "", "", 
                      "How", "to", "Play", "", ""
                      "", "Use", "the", "directional", "keys", "to", "move", "the", "tiles.", "When", "two", "tiles", "with", "the", "same", "number", "touch,", "they", "merge", "into", "one!"};
  struct style_text style[]={
    {0, TEXT_CENTER|TEXT_UNDERLINE},
    {2, C_RED},
    {15, C_RED}, {16, C_RED}, {17,C_RED},
    LAST_STYLE_ITEM
  };
  return display_text(ARRAYLEN(help_text), help_text, style,NULL,true);
}
static inline void slide_internal(int startx, int starty, int stopx, int stopy, int dx, int dy, int lookx, int looky)
{
  for(int y=starty;y!=stopy;y+=dy)
    {
      for(int x=startx;x!=stopx;x+=dx)
        {
          if(ctx->grid[x+lookx][y+looky]==ctx->grid[x][y] && ctx->grid[x][y] && !merged_grid[x+lookx][y+looky] && !merged_grid[x][y]) /* Slide into */
            {
              /* Each merged tile cannot be merged again */
              merged_grid[x+lookx][y+looky]=true;
              ctx->grid[x+lookx][y+looky]=2*ctx->grid[x][y];
              ctx->score+=ctx->grid[x+lookx][y+looky];
              ctx->grid[x][y]=0;
            }
          else if(ctx->grid[x+lookx][y+looky]==0) /* Empty! */
            {
              ctx->grid[x+lookx][y+looky]=ctx->grid[x][y];
              ctx->grid[x][y]=0;
            }
        }
    }
}
/* Up
   0 
   1 ^ ^ ^ ^
   2 ^ ^ ^ ^
   3 ^ ^ ^ ^
     0 1 2 3
*/
static void up(void)
{
  slide_internal(0, 1,  /* start values */
                 4, 4, /* stop values */
                 1, 1, /* delta values */
                 0, -1); /* lookahead values */
}
/* Down
   0 v v v v
   1 v v v v
   2 v v v v
   3 
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
   0   < < <
   1   < < <
   2   < < <
   3   < < <
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
   0 > > >  
   1 > > >  
   2 > > >  
   3 > > >  
     0 1 2 3
*/ 
static void right(void)
{
  slide_internal(2, 0, /* start */
                 -1, 4, /* stop */
                 -1, 1, /* delta */
                 1, 0); /* lookahead */
}
static void draw(void)
{
  rb->lcd_clear_display();
  /* Draw the ctx->grid */
  char str[32];
  for(int y=3;y>=0;--y)
    {
      for(int x=0;x<4;++x)
        {
          if(ctx->grid[x][y])
            {
              rb->snprintf(str,31,"%d", ctx->grid[x][y]);
              rb->lcd_putsxy(winning_tile_length*x,y*max_numeral_height+max_numeral_height,str);
            }
        }
    }
  /* Now draw the score, and the game title */
  rb->snprintf(str, 31, "Score: %d", ctx->score);
  int str_width, str_height;
  rb->font_getstringsize(str, &str_width, &str_height, WHAT_FONT);
  int score_leftmost=LCD_WIDTH-str_width-1;
  /* Check if there is enough space to display "Score: ", otherwise, only display the score */
  if(score_leftmost>=0)
    rb->lcd_putsxy(score_leftmost,0,str);
  else
    rb->lcd_putsxy(score_leftmost,0,str+rb->strlen("Score: "));
  /* Reuse the same string for the title */
  
  rb->snprintf(str, 31, "%d", WINNING_TILE);
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
        if(!ctx->grid[x][y])
          {
            xpos[back]=x;
            ypos[back]=y;
            ++back;
          }
      }
  if(!back)
    return;
  int idx=rand_range(0,back-1);
  ctx->grid[xpos[idx]][ypos[idx]]=rand_2_or_4();
}
static void restore_old_grid(void)
{
  memcpy(&ctx->grid, &old_grid, sizeof(int)*16);
}
static bool check_gameover(void)
{
  int numempty=0;
  for(int y=0;y<4;++y)
    {
      for(int x=0;x<4;++x)
        {
          if(ctx->grid[x][y]==0)
            ++numempty;
          if(ctx->grid[x][y]==WINNING_TILE)
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
      int oldscore=ctx->score;
      memset(&merged_grid,0,16*sizeof(bool));
      up();
      if(memcmp(&old_grid, &ctx->grid, sizeof(int)*16))
        {
          restore_old_grid();
          ctx->score=oldscore;
          return false;
        }
      restore_old_grid();
      memset(&merged_grid,0,16*sizeof(bool));
      down();
      if(memcmp(&old_grid, &ctx->grid, sizeof(int)*16))
        {
          restore_old_grid();
          ctx->score=oldscore; 
          return false;
        }
      restore_old_grid();
      memset(&merged_grid,0,16*sizeof(bool));
      left();
      if(memcmp(&old_grid, &ctx->grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          ctx->score=oldscore;
          return false;
        }
      restore_old_grid();
      memset(&merged_grid,0,16*sizeof(bool));
      right();
      if(memcmp(&old_grid, &ctx->grid, sizeof(int)*16))
        {
          restore_old_grid(); 
          ctx->score=oldscore;
          return false;
        }
      ctx->score=oldscore;
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
static void init_game(bool newgame)
{
  if(newgame)
    {
      /* initialize the game context */
      for(int y=0;y<4;++y)
        {
          for(int x=0;x<4;++x)
            ctx->grid[x][y]=0;
        }
      for(int i=0;i<MAX_STARTING_TILES;++i)
        {
          place_random();
        }
      ctx->score=0;
    }
  /* Now calculate font sizes, etc */
  max_numeral_width=-1;
  max_numeral_height=-1;
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
  int fd=rb->open(RESUME_FILE,O_WRONLY|O_CREAT, 0666);
  if(fd<0)
    {
      rb->splash(2*HZ, "Saving failed.");
      return;
   }
  rb->write(fd, ctx,sizeof(struct game_ctx_t));
  rb->close(fd);
}
static bool load_game(void)
{
  bool success=false;
  int fd=rb->open(RESUME_FILE, O_RDONLY);
  if(fd<0)
    {
      rb->remove(RESUME_FILE);
      return false;
    }
  int numread=rb->read(fd, ctx, sizeof(struct game_ctx_t));
  if(numread==sizeof(struct game_ctx_t))
    success=true;
  rb->close(fd);
  rb->remove(RESUME_FILE);
  return success;
}
/* load/save code is based off BlackJack */
static int do_2048_pause_menu(void)
{
  int sel=0;
  MENUITEM_STRINGLIST(menu,"Paused", NULL, 
                      "Resume Game", 
                      "Start New Game",
                      "High Scores",
                      "Playback Control", 
                      "Help",
                      "Quit without Saving", 
                      "Save and Quit");
  bool quit=false;
  while(!quit)
    {
      switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
          draw();
          return 0;
        case 1:
          return 1;
        case 2:
          highscore_show(-1,highscores, NUM_SCORES, true);
          break;
        case 3:
          playback_control(NULL);
          break;
        case 4:
          do_help();
          break;
        case 5:
          return 2;
        case 6:
          return 3;
        }
    }
  return 0;
}
static enum plugin_status do_game(bool newgame)
{
  init_game(newgame);
  int made_move=0;
  while(1)
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(false); /* Save battery when idling */
#endif
      /* Wait for a button press */
      int button = rb->button_get(true);
      made_move=0;
      memset(&merged_grid,0,16*sizeof(bool));
      memcpy(&old_grid, &ctx->grid, sizeof(int)*16);
      int grid_before_anim_step[4][4];
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(true); /* doing work now... */
#endif
      switch(button)
        {
        case BUTTON_UP:
          for(int i=0;i<3;++i)
            {
              memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*16);
              up();
              if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*16))
                {
                  rb->sleep(ANIM_SLEEPTIME);
                  draw();
                }
            }
          made_move=1;
          break;
        case BUTTON_DOWN:
          for(int i=0;i<3;++i)
            {
              memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*16);
              down();
              if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*16))
                {
                  rb->sleep(ANIM_SLEEPTIME);              
                  draw();
                }
            }
          made_move=1;
          break;
        case BUTTON_LEFT:
          for(int i=0;i<3;++i)
            {
              memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*16);
              left();
              if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*16))
                {
                  rb->sleep(ANIM_SLEEPTIME);              
                  draw();
                }
            }
          made_move=1;
          break;
        case BUTTON_RIGHT:
          for(int i=0;i<3;++i)
            {
              memcpy(grid_before_anim_step, ctx->grid, sizeof(int)*16);
              right();
              if(memcmp(grid_before_anim_step, ctx->grid, sizeof(int)*16))
                {
                  rb->sleep(ANIM_SLEEPTIME);              
                  draw();
                }
            }
          made_move=1;
          break;
        case BUTTON_POWER:
          switch(do_2048_pause_menu())
            {
            case 0: /* resume */
              break;
            case 1: /* new game */
              init_game(true);
              made_move=1;
              continue;
              break;
            case 2: /* quit without saving */
              rb->remove(RESUME_FILE);
              return PLUGIN_ERROR;
            case 3: /* save and quit */
              save_game();
              return (enum plugin_status)0x80;
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
          if(memcmp(&old_grid, ctx->grid, sizeof(int)*16))
            place_random();
          memcpy(&old_grid, ctx->grid, sizeof(int)*16);
          if(check_gameover())
            return PLUGIN_OK;
          draw();
        }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(false); /* back to idle */
#endif
      rb->yield();
    }
}
static void hs_check_update(bool noshow)
{
  /* first, find the biggest tile */
  int biggest=0;
  for(int x=0;x<4;++x)
    {
      for(int y=0;y<4;++y)
        {
          if(ctx->grid[x][y]>biggest)
            biggest=ctx->grid[x][y];
        }
    }
  int hs_idx=highscore_update(ctx->score,biggest, "", highscores,NUM_SCORES);
  if(!noshow)
    {
      if(hs_idx>=0)
        rb->splash(HZ*2,"High Score!");
      rb->lcd_clear_display();
      highscore_show(hs_idx,highscores,NUM_SCORES,true);
    }
  highscore_save(HISCORES_FILE,highscores,NUM_SCORES);
}
static int mainmenu_shouldshow(int action, const struct menu_item_ex *this_item)
{
  int idx=((intptr_t)this_item);
  if(action==ACTION_REQUEST_MENUITEM && !loaded && (idx==0 || idx==5))
    return ACTION_EXIT_MENUITEM;
  return action;
}
static enum plugin_status do_2048_menu(void)
{
  int sel=0;
  loaded=load_game();
  MENUITEM_STRINGLIST(menu,"2048 Menu", mainmenu_shouldshow, "Resume Game", "Start New Game","High Scores","Playback Control", "Help", "Quit without Saving", "Quit");
  bool quit=false;
  while(!quit)
    {
      int item;
      switch(item=rb->do_menu(&menu,&sel,NULL,false))
        {
        case 0: /* Start new game or resume a game */
        case 1:
          {
            enum plugin_status ret=do_game(item==1);
            switch(ret)
              {
              case PLUGIN_OK:
                {
                  loaded=false;
                  rb->remove(RESUME_FILE);
                  hs_check_update(false);
                  break;
                }
              case PLUGIN_USB_CONNECTED:
                save_game();
                /* Don't bother showing the high scores... */
                return ret;
              case PLUGIN_ERROR: /* exit without menu */
                hs_check_update(false);
                return PLUGIN_OK;
              case (enum plugin_status)0x80: /* exit wo/ hs */
                return PLUGIN_OK;
              default:
                break;
              }
            break;
          }
        case 2:
          highscore_show(-1,highscores, NUM_SCORES, true);
          break;
        case 3:
          playback_control(NULL);
          break;
        case 4:
          do_help();
          break;
        case 5:
          return PLUGIN_OK;
        case 6:
          if(loaded)
            save_game();
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
