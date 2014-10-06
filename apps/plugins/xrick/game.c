/*
 * xrick/game.c
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net).
 * Copyright (C) 2008-2014 Pierluigi Vicinanza.
 * All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#include "xrick/game.h"

#include "xrick/draw.h"
#include "xrick/maps.h"
#include "xrick/ents.h"
#include "xrick/e_rick.h"
#include "xrick/e_sbonus.h"
#include "xrick/e_them.h"
#include "xrick/screens.h"
#include "xrick/rects.h"
#include "xrick/scroller.h"
#include "xrick/control.h"
#include "xrick/resources.h"

#ifdef ENABLE_DEVTOOLS
#include "xrick/devtools.h"
#endif


/*
 * local typedefs
 */
typedef enum {
#ifdef ENABLE_DEVTOOLS
  DEVTOOLS,
#endif
  XRICK,
  INIT_GAME, INIT_BUFFER,
  INTRO_MAIN, INTRO_MAP,
  PAUSE_PRESSED1, PAUSE_PRESSED1B, PAUSED, PAUSE_PRESSED2,
  PLAY0, PLAY1, PLAY2, PLAY3,
  CHAIN_SUBMAP, CHAIN_MAP, CHAIN_END,
  SCROLL_UP, SCROLL_DOWN,
  RESTART, GAMEOVER, GETNAME, EXIT
} game_state_t;


/*
 * global vars
 */
U8 game_period = 0;
bool game_waitevt = false;
const rect_t *game_rects = NULL;

U8 game_lives = 0;
U8 game_bombs = 0;
U8 game_bullets = 0;
U32 game_score = 0;

U16 game_map = 0;
U16 game_submap = 0;

U8 game_dir = RIGHT;
bool game_chsm = false;

bool game_cheat1 = false;
bool game_cheat2 = false;
bool game_cheat3 = false;


/*
 * local vars
 */
static U8 isave_frow;
static game_state_t game_state;
#ifdef ENABLE_SOUND
static sound_t *currentMusic = NULL;
#endif


/*
 * prototypes
 */
static void frame(void);
static void init(void);
static void play0(void);
static void play3(void);
static void restart(void);
static void isave(void);
static void irestore(void);


/*
 * Cheats
 */
#ifdef ENABLE_CHEATS
void
game_toggleCheat(cheat_t cheat)
{
    if (game_state != INTRO_MAIN && game_state != INTRO_MAP &&
        game_state != GAMEOVER && game_state != GETNAME &&
#ifdef ENABLE_DEVTOOLS
        game_state != DEVTOOLS &&
#endif
        game_state != XRICK && game_state != EXIT)
    {
        switch (cheat)
        {
            case Cheat_UNLIMITED_ALL:
            {
                game_cheat1 = !game_cheat1;
                game_lives = 6;
                game_bombs = 6;
                game_bullets = 6;
                break;
            }
            case Cheat_NEVER_DIE:
            {
                game_cheat2 = !game_cheat2;
                break;
            }
            case Cheat_EXPOSE:
            {
                game_cheat3 = !game_cheat3;
                break;
            }
        }
        draw_infos();
        /* FIXME this should probably only raise a flag ... */
        /* plus we only need to update INFORECT not the whole screen */
        sysvid_update(&draw_SCREENRECT);
    }
}
#endif

#ifdef ENABLE_SOUND
/*
 * Music
 */
void
game_setmusic(sound_t * newMusic, S8 loop)
{
    if (!newMusic)
    {
        return;
    }

    if (currentMusic)
    {
        game_stopmusic();
    }

    syssnd_play(newMusic, loop);

    currentMusic = newMusic;
}

void
game_stopmusic(void)
{
    syssnd_stop(currentMusic);
    currentMusic = NULL;
}
#endif /*ENABLE_SOUND */

/*
 * Main loop
 */
void
game_run(void)
{
    U32 currentTime,
#ifdef ENABLE_SOUND
        lastSoundTime = 0,
#endif
        lastFrameTime = 0;

    if (!resources_load())
    {
        resources_unload();
        return;
    }

    if (!sys_cacheData())
    {
        sys_uncacheData();
        return;
    }

    game_period = sysarg_args_period ? sysarg_args_period : GAME_PERIOD;
    game_state = XRICK;

    /* main loop */
    while (game_state != EXIT)
    {
        currentTime = sys_gettime();

        if (currentTime - lastFrameTime >= game_period)
        {
            /* frame */
            frame();

            /* video */
            /*DEBUG*//*game_rects=&draw_SCREENRECT;*//*DEBUG*/
            sysvid_update(game_rects);

            /* reset rectangles list */
            rects_free(ent_rects);
            ent_rects = NULL;
            draw_STATUSRECT.next = NULL;  /* FIXME freerects should handle this */

            /* events */
            if (game_waitevt)
            {
                sysevt_wait();  /* wait for an event */
            }
            else
            {
                sysevt_poll();  /* process events (non-blocking) */
            }

            lastFrameTime = currentTime;
        }

#ifdef ENABLE_SOUND
        if (currentTime - lastSoundTime >= syssnd_period)
        {
            /* sound */
            syssnd_update();

            lastSoundTime = currentTime;
        }
#endif /* ENABLE_SOUND */

        sys_yield();
    }

#ifdef ENABLE_SOUND
    syssnd_stopAll();
#endif

    sys_uncacheData();

    resources_unload();
}

/*
 * Prepare frame
 *
 * This function loops forever: use 'return' when a frame is ready.
 * When returning, game_rects must contain every parts of the buffer
 * that have been modified.
 */
static void
frame(void)
{
    while (1) {

        switch (game_state) {



#ifdef ENABLE_DEVTOOLS
        case DEVTOOLS:
            switch (devtools_run()) {
            case SCREEN_RUNNING:
                return;
            case SCREEN_DONE:
                game_state = INIT_GAME;
                break;
            case SCREEN_EXIT:
                game_state = EXIT;
                return;
            }
        break;
#endif



        case XRICK:
            switch(screen_xrick()) {
            case SCREEN_RUNNING:
                return;
            case SCREEN_DONE:
#ifdef ENABLE_DEVTOOLS
                game_state = DEVTOOLS;
#else
                game_state = INIT_GAME;
#endif
                break;
            case SCREEN_EXIT:
                game_state = EXIT;
                return;
            }
        break;



        case INIT_GAME:
            init();
            game_state = INTRO_MAIN;
            break;



        case INTRO_MAIN:
            switch (screen_introMain()) {
            case SCREEN_RUNNING:
                return;
            case SCREEN_DONE:
                game_state = INTRO_MAP;
                break;
            case SCREEN_EXIT:
                game_state = EXIT;
                return;
            }
        break;



        case INTRO_MAP:
            switch (screen_introMap()) {
            case SCREEN_RUNNING:
                return;
            case SCREEN_DONE:
                game_waitevt = false;
                game_state = INIT_BUFFER;
                break;
            case SCREEN_EXIT:
                game_state = EXIT;
                return;
            }
        break;



        case INIT_BUFFER:
            sysvid_clear();                 /* clear buffer */
            draw_map();                     /* draw the map onto the buffer */
            draw_drawStatus();              /* draw the status bar onto the buffer */
#ifdef ENABLE_CHEATS
            draw_infos();                   /* draw the info bar onto the buffer */
#endif
            game_rects = &draw_SCREENRECT;  /* request full buffer refresh */
            game_state = PLAY0;
            return;



        case PAUSE_PRESSED1:
            screen_pause(true);
            game_state = PAUSE_PRESSED1B;
            break;



        case PAUSE_PRESSED1B:
            if (control_test(Control_PAUSE))
                return;
            game_state = PAUSED;
            break;



        case PAUSED:
            if (control_test(Control_PAUSE))
                game_state = PAUSE_PRESSED2;
            if (control_test(Control_EXIT))
                game_state = EXIT;
            return;



        case PAUSE_PRESSED2:
            if (!(control_test(Control_PAUSE))) {
                game_waitevt = false;
                screen_pause(false);
#ifdef ENABLE_SOUND
                syssnd_pauseAll(false);
#endif
                game_state = PLAY2;
            }
        return;



        case PLAY0:
            play0();
            break;



        case PLAY1:
            if (control_test(Control_PAUSE)) {
#ifdef ENABLE_SOUND
                syssnd_pauseAll(true);
#endif
                game_waitevt = true;
                game_state = PAUSE_PRESSED1;
            }
            else if (!control_active) {
#ifdef ENABLE_SOUND
                syssnd_pauseAll(true);
#endif
                game_waitevt = true;
                screen_pause(true);
                game_state = PAUSED;
            }
            else
                game_state = PLAY2;
            break;



        case PLAY2:
            if (e_rick_state_test(E_RICK_STDEAD)) {  /* rick is dead */
                if (game_cheat1 || --game_lives) {
                    game_state = RESTART;
                } else {
                    game_state = GAMEOVER;
                }
            }
            else if (game_chsm)  /* request to chain to next submap */
                game_state = CHAIN_SUBMAP;
            else
                game_state = PLAY3;
            break;



    case PLAY3:
      play3();
      return;



    case CHAIN_SUBMAP:
      if (map_chain())
    game_state = CHAIN_END;
      else {
    game_bullets = 0x06;
    game_bombs = 0x06;
    game_map++;

    if (game_map == map_nbr_maps - 1) {
      /* reached end of game */
      /* FIXME @292?*/
    }

    game_state = CHAIN_MAP;
      }
      break;



    case CHAIN_MAP:                             /* CHAIN MAP */
      switch (screen_introMap()) {
      case SCREEN_RUNNING:
    return;
      case SCREEN_DONE:
    if (game_map >= map_nbr_maps - 1) {  /* reached end of game */
      sysarg_args_map = 0;
      sysarg_args_submap = 0;
      game_state = GAMEOVER;
    }
    else {  /* initialize game */
      ent_ents[1].x = map_maps[game_map].x;
      ent_ents[1].y = map_maps[game_map].y;
      map_frow = (U8)map_maps[game_map].row;
      game_submap = map_maps[game_map].submap;
      game_state = CHAIN_END;
    }
    break;
      case SCREEN_EXIT:
    game_state = EXIT;
    return;
      }
      break;



    case CHAIN_END:
      map_init();                     /* initialize the map */
      isave();                        /* save data in case of a restart */
      ent_clprev();                   /* cleanup entities */
      draw_map();                     /* draw the map onto the buffer */
      draw_drawStatus();              /* draw the status bar onto the buffer */
      game_rects = &draw_SCREENRECT;  /* request full screen refresh */
      game_state = PLAY0;
      return;



    case SCROLL_UP:
      switch (scroll_up()) {
      case SCROLL_RUNNING:
    return;
      case SCROLL_DONE:
    game_state = PLAY0;
    break;
      }
      break;



    case SCROLL_DOWN:
      switch (scroll_down()) {
      case SCROLL_RUNNING:
    return;
      case SCROLL_DONE:
    game_state = PLAY0;
    break;
      }
      break;



    case RESTART:
      restart();
      game_state = PLAY0;
      return;



    case GAMEOVER:
      switch (screen_gameover()) {
      case SCREEN_RUNNING:
    return;
      case SCREEN_DONE:
    game_state = GETNAME;
    break;
      case SCREEN_EXIT:
    game_state = EXIT;
    break;
      }
      break;



    case GETNAME:
      switch (screen_getname()) {
      case SCREEN_RUNNING:
    return;
      case SCREEN_DONE:
    game_state = INIT_GAME;
    return;
      case SCREEN_EXIT:
    game_state = EXIT;
    break;
      }
      break;



    case EXIT:
      return;

    }
  }
}


/*
 * Initialize the game
 */
static void
init(void)
{
  U8 i;

  e_rick_state_clear(0xff);

  game_lives = 6;
  game_bombs = 6;
  game_bullets = 6;
  game_score = 0;

  game_map = sysarg_args_map;

  if (sysarg_args_submap == 0)
  {
      game_submap = map_maps[game_map].submap;
      map_frow = (U8)map_maps[game_map].row;
  }
  else
  {
      /* dirty hack to determine frow */
      game_submap = sysarg_args_submap;
      i = 0;
      while (i < map_nbr_connect &&
            (map_connect[i].submap != game_submap ||
             map_connect[i].dir != RIGHT))
      {
          i++;
      }
      map_frow = map_connect[i].rowin - 0x10;
      ent_ents[1].y = 0x10 << 3;
  }

  ent_ents[1].x = map_maps[game_map].x;
  ent_ents[1].y = map_maps[game_map].y;
  ent_ents[1].w = 0x18;
  ent_ents[1].h = 0x15;
  ent_ents[1].n = 0x01;
  ent_ents[1].sprite = 0x01;
  ent_ents[1].front = false;
  ent_ents[ENT_ENTSNUM].n = 0xFF;

  map_resetMarks();

  map_init();
  isave();
}


/*
 * play0
 *
 */
static void
play0(void)
{
    if (control_test(Control_END)) {  /* request to end the game */
        game_state = GAMEOVER;
        return;
    }

    if (control_test(Control_EXIT)) {  /* request to exit the game */
        game_state = EXIT;
        return;
    }

    ent_action();      /* run entities */
    e_them_rndseed++;  /* (0270) */

    game_state = PLAY1;
}


/*
 * play3
 *
 */
static void
play3(void)
{
    draw_clearStatus();  /* clear the status bar */
    ent_draw();          /* draw all entities onto the buffer */
    /* sound */
    draw_drawStatus();   /* draw the status bar onto the buffer*/

    game_rects = &draw_STATUSRECT; /* refresh status bar too */
    draw_STATUSRECT.next = ent_rects;  /* take care to cleanup draw_STATUSRECT->next later! */

    if (!e_rick_state_test(E_RICK_STZOMBIE)) {  /* need to scroll ? */
        if (ent_ents[1].y >= 0xCC) {
            game_state = SCROLL_UP;
            return;
        }
        if (ent_ents[1].y <= 0x60) {
            game_state = SCROLL_DOWN;
            return;
        }
    }

    game_state = PLAY0;
}


/*
 * restart
 *
 */
static void
restart(void)
{
  e_rick_state_clear(E_RICK_STDEAD|E_RICK_STZOMBIE);

  game_bullets = 6;
  game_bombs = 6;

  ent_ents[1].n = 1;

  irestore();
  map_init();
  isave();
  ent_clprev();
  draw_map();
  draw_drawStatus();
  game_rects = &draw_SCREENRECT;
}


/*
 * isave (0bbb)
 *
 */
static void
isave(void)
{
  e_rick_save();
  isave_frow = map_frow;
}


/*
 * irestore (0bdc)
 *
 */
static void
irestore(void)
{
  e_rick_restore();
  map_frow = isave_frow;
}

/* eof */
