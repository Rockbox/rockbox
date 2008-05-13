/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * robotfindskitten: A Zen simulation
 *
 * Copyright (C) 1997,2000 Leonard Richardson 
 *                         leonardr@segfault.org
 *                         http://www.crummy.com/devel/
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or EXISTANCE OF KITTEN.  See the GNU General
 *   Public License for more details.
 *
 *   http://www.gnu.org/copyleft/gpl.html
 *
 * Ported to Rockbox 2007 by Jonas Häggqvist
 */

#include "plugin.h"
#include "pluginlib_actions.h"

/* This macros must always be included. Should be placed at the top by
   convention, although the actual position doesn't matter */
PLUGIN_HEADER

/*The messages go in a separate file because they are collectively
  huge, and you might want to modify them. It would be nice to load
  the messages from a text file at run time.*/
#include "robotfindskitten_messages.h"

#define TRUE true
#define FALSE false

#define RFK_VERSION "v1.4142135.406"

/* Button definitions stolen from maze.c */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   undef __PLUGINLIB_ACTIONS_H__
#   define RFK_QUIT     (BUTTON_SELECT | BUTTON_MENU)
#   define RFK_RIGHT    BUTTON_RIGHT
#   define RFK_LEFT     BUTTON_LEFT
#   define RFK_UP       BUTTON_MENU
#   define RFK_DOWN     BUTTON_PLAY
#   define RFK_RRIGHT   (BUTTON_RIGHT | BUTTON_REPEAT)
#   define RFK_RLEFT    (BUTTON_LEFT | BUTTON_REPEAT)
#   define RFK_RUP      (BUTTON_MENU | BUTTON_REPEAT)
#   define RFK_RDOWN    (BUTTON_PLAY | BUTTON_REPEAT)

#else
#   define RFK_QUIT     PLA_QUIT
#   define RFK_RIGHT    PLA_RIGHT
#   define RFK_LEFT     PLA_LEFT
#   define RFK_UP       PLA_UP
#   define RFK_DOWN     PLA_DOWN
#   define RFK_RRIGHT   PLA_RIGHT_REPEAT
#   define RFK_RLEFT    PLA_LEFT_REPEAT
#   define RFK_RUP      PLA_UP_REPEAT
#   define RFK_RDOWN    PLA_DOWN_REPEAT

#endif
/*Constants for our internal representation of the screen.*/
#define EMPTY -1
#define ROBOT 0
#define KITTEN 1

/*Screen dimensions.*/
#define X_MIN 0
#define X_MAX ((LCD_WIDTH/SYSFONT_WIDTH) - 1)
#define Y_MIN 3
#define Y_MAX ((LCD_HEIGHT/SYSFONT_HEIGHT) - 1)

/* Colours used */
#if LCD_DEPTH >= 16
#define NUM_COLORS 6
#define ROBOT_COLOR LCD_DARKGRAY
const unsigned colors[NUM_COLORS] = {
    LCD_RGBPACK(255, 255, 0), /* Yellow */
    LCD_RGBPACK(0, 255, 255), /* Cyan */
    LCD_RGBPACK(255, 0, 255), /* Purple */
    LCD_RGBPACK(0, 0, 255), /* Blue */
    LCD_RGBPACK(255, 0, 0), /* Red */
    LCD_RGBPACK(0, 255, 0), /* Green */
};
#elif LCD_DEPTH == 2
#define NUM_COLORS 3
#define ROBOT_COLOR LCD_DARKGRAY
const unsigned colors[NUM_COLORS] = {
    LCD_LIGHTGRAY,
    LCD_DARKGRAY,
    LCD_BLACK,
};
#elif LCD_DEPTH == 1
#define NUM_COLORS 1
#define ROBOT_COLOR 0
const unsigned colors[NUM_COLORS] = {
    0,
};
#endif /* HAVE_LCD_COLOR */

/*Macros for generating numbers in different ranges*/
#define randx() (rb->rand() % X_MAX) + 1
#define randy() (rb->rand() % (Y_MAX-Y_MIN+1))+Y_MIN /*I'm feeling randy()!*/
#define randchar() rb->rand() % (126-'!'+1)+'!';
#define randcolor() rb->rand() % NUM_COLORS
#define randbold() (rb->rand() % 2 ? TRUE:FALSE)

/*Row constants for the animation*/
#define ADV_ROW 1
#define ANIMATION_MEET (X_MAX/3)*2
#define ANIMATION_LENGTH 4

/*This struct contains all the information we need to display an object
  on the screen*/
typedef struct
{
  short x;
  short y;
  int color;
  bool bold;
  char character;
} screen_object;

/*
 *Function definitions
 */

/*Initialization and setup functions*/
static void initialize_arrays(void);
static void initialize_robot(void);
static void initialize_kitten(void);
static void initialize_bogus(void);
static void initialize_screen(void);
static void instructions(void);
static void finish(int sig);

/*Game functions*/
static void play_game(void);
static void process_input(int);

/*Helper functions*/
static void pause(void);
static int validchar(char);
static void play_animation(int);

/*Global variables. Bite me, it's fun.*/
screen_object robot;
screen_object kitten;

#if X_MAX*Y_MAX < 200
#define NUM_BOGUS 15
#else
#define NUM_BOGUS 20
#endif
screen_object bogus[NUM_BOGUS];
unsigned short bogus_messages[NUM_BOGUS];
bool used_messages[MESSAGES];

bool exit_rfk;

/* This array contains our internal representation of the screen. The
 array is bigger than it needs to be, as we don't need to keep track
 of the first few rows of the screen. But that requires making an
 offset function and using that everywhere. So not right now. */
int screen[X_MAX + 1][Y_MAX + 1];

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static const struct plugin_api* rb;

/******************************************************************************
 *
 * Begin meaty routines that do the dirty work.
 *
 *****************************************************************************/

MEM_FUNCTION_WRAPPERS(rb) 

static void drawchar(int x, int y, char c)
{
  char str[2];
  rb->snprintf(str, sizeof(str), "%c", c);
  rb->lcd_putsxy(x*SYSFONT_WIDTH, y*SYSFONT_HEIGHT, str);
}

static void draw(screen_object o)
{
#if LCD_DEPTH > 1
  unsigned oldforeground;
  oldforeground = rb->lcd_get_foreground();
  rb->lcd_set_foreground(o.color);
  drawchar(o.x, o.y, o.character);
  rb->lcd_set_foreground(oldforeground);
#else
  drawchar(o.x, o.y, o.character);
#endif
}

static void message(char * str)
{
  rb->lcd_puts_scroll(0, ADV_ROW, str);
}

static void refresh(void)
{
  rb->lcd_update();
}

/*
 *play_game waits in a loop getting input and sending it to process_input
 */
static void play_game()
{
  int old_x = robot.x;
  int old_y = robot.y;
  int input = 0; /* Not sure what a reasonable initial value is */
#ifdef __PLUGINLIB_ACTIONS_H__
  const struct button_mapping *plugin_contexts[] = {generic_directions, generic_actions};
#endif

  while (input != RFK_QUIT && exit_rfk == false)
    {
      process_input(input);
      
      /*Redraw robot, where applicable. We're your station, robot.*/
      if (!(old_x == robot.x && old_y == robot.y))
    {
      /*Get rid of the old robot*/
    drawchar(old_x, old_y, ' ');
      screen[old_x][old_y] = EMPTY;
      
      /*Meet the new robot, same as the old robot.*/
      draw(robot);
      refresh();
      screen[robot.x][robot.y] = ROBOT;
      
      old_x = robot.x;
      old_y = robot.y;
    }
#ifdef __PLUGINLIB_ACTIONS_H__
      input = pluginlib_getaction(rb, TIMEOUT_BLOCK, plugin_contexts, 2);
#else
      input = rb->button_get(true);
#endif
    } 
  message("Bye!");
  refresh();
}

/*
 *Given the keyboard input, process_input interprets it in terms of moving,
 *touching objects, etc.
 */
static void process_input(int input)
{
  int check_x = robot.x;
  int check_y = robot.y;

  switch (input)
    {
    case RFK_UP:
    case RFK_RUP:
      check_y--;
      break;
    case RFK_DOWN:
    case RFK_RDOWN:
      check_y++;
      break;
    case RFK_LEFT:
    case RFK_RLEFT:
      check_x--;
      break;
    case RFK_RIGHT:
    case RFK_RRIGHT:
      check_x++;
      break;
    }
  
  /*Check for going off the edge of the screen.*/
  if (check_y < Y_MIN || check_y > Y_MAX || check_x < X_MIN || check_x > X_MAX)
    {
      return; /*Do nothing.*/
    }

  /*
   * Clear textline
   * disabled because it breaks the scrolling for some reason
   */
  /* rb->lcd_puts_scroll(0, ADV_ROW, " "); */
  
  /*Check for collision*/
  if (screen[check_x][check_y] != EMPTY)
    {
      switch (screen[check_x][check_y])
      {
        case ROBOT:
              /*We didn't move, or we're stuck in a
                time warp or something.*/
          break;
        case KITTEN: /*Found it!*/
          play_animation(input);
          /* Wait for the user to click something */
          pause();
          break;
        default: /*We hit a bogus object; print its message.*/
          message(messages[bogus_messages[screen[check_x][check_y]-2]]);
          refresh();
          break;
      }
      return;
    }

  /*Otherwise, move the robot.*/
  robot.x = check_x;
  robot.y = check_y;
}

/*finish is called upon signal or progam exit*/
static void finish(int sig)
{
  (void)sig;
  exit_rfk = true;
}

/******************************************************************************
 *
 * Begin helper routines
 *
 *****************************************************************************/

static void pause()
{
  int button;
  rb->lcd_update();
  do
    button = rb->button_get(true);
  while( ( button == BUTTON_NONE )
      || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
}

static int validchar(char a)
{
  switch(a)
    {
    case '#':
    case ' ':   
    case 127:
      return 0;
    }
  return 1;
}

static void play_animation(int input)
{
  int counter;
  screen_object left;
  screen_object right;
  /*The grand cinema scene.*/
  rb->lcd_puts_scroll(0, ADV_ROW, " ");

  if (input == RFK_RIGHT || input == RFK_DOWN || input == RFK_RRIGHT || input == RFK_RDOWN) {
    left = robot;
    right = kitten;
  }
  else {
    left = kitten;
    right = robot;
  }
  left.y = ADV_ROW;
  right.y = ADV_ROW;
  left.x = ANIMATION_MEET - ANIMATION_LENGTH - 1;
  right.x = ANIMATION_MEET + ANIMATION_LENGTH;

  for (counter = ANIMATION_LENGTH; counter > 0; counter--)
  {
    left.x++;
    right.x--;
    /* Clear the previous position (empty the first time) */
    drawchar(left.x - 1, left.y, ' ');
    drawchar(right.x + 1, right.y, ' ');
    draw(left);
    draw(right);
    refresh();
    rb->sleep(HZ);
  }

  message("You found kitten! Way to go, robot!");
  refresh();
  finish(0);
}

/******************************************************************************
 *
 * Begin initialization routines (called before play begins).
 *
 *****************************************************************************/

static void instructions()
{
#define MARGIN 2
  int y = MARGIN, space_w, width, height;
  unsigned short x = MARGIN, i = 0;
#define WORDS (sizeof instructions / sizeof (char*))
  static char* instructions[] = {
#if 0
    /* Not sure if we want to include this? */
    "robotfindskitten", RFK_VERSION, "", "",
    "By", "the", "illustrious", "Leonard", "Richardson", "(C)", "1997,", "2000", "",
    "Written", "originally", "for", "the", "Nerth", "Pork", "robotfindskitten", "contest", "", "",
#endif
    "In", "this", "game", "you", "are", "robot", "(#).", "Your", "job", "is", "to", "find", "kitten.", "This", "task", "is", "complicated", "by", "the", "existence", "of", "various", "things", "which", "are", "not", "kitten.", "Robot", "must", "touch", "items", "to", "determine", "if", "they", "are", "kitten", "or", "not.", "",
    "The", "game", "ends", "when", "robotfindskitten.", "", "",
    "Press", "any", "key", "to", "start",
  };
  rb->lcd_clear_display();
  rb->lcd_getstringsize(" ", &space_w, &height);
  for (i = 0; i < WORDS; i++) {
    rb->lcd_getstringsize(instructions[i], &width, NULL);
    /* Skip to next line if the current one can't fit the word */
    if (x + width > LCD_WIDTH - MARGIN) {
      x = MARGIN;
      y += height;
    }
    /* .. or if the word is the empty string */
    if (rb->strcmp(instructions[i], "") == 0) {
      x = MARGIN;
      y += height;
      continue;
    }
    /* We filled the screen */
    if (y + height > LCD_HEIGHT - MARGIN) {
      y = MARGIN;
      pause();
      rb->lcd_clear_display();
    }
    rb->lcd_putsxy(x, y, instructions[i]);
    x += width + space_w;
  }
  pause();
}

static void initialize_arrays()
{
  unsigned int counter, counter2;
  screen_object empty;

  /*Initialize the empty object.*/
  empty.x = -1;
  empty.y = -1;
#if LCD_DEPTH > 1
  empty.color = LCD_BLACK;
#else
  empty.color = 0;
#endif
  empty.bold = FALSE;
  empty.character = ' ';
  
  for (counter = 0; counter <= X_MAX; counter++)
  {
    for (counter2 = 0; counter2 <= Y_MAX; counter2++)
    {
      screen[counter][counter2] = EMPTY;
    }
  }
  
  /*Initialize the other arrays.*/
  for (counter = 0; counter < MESSAGES; counter++)
  {
    used_messages[counter] = false;
  }
  for (counter = 0; counter < NUM_BOGUS; counter++)
  {
    bogus_messages[counter] = 0;
    bogus[counter] = empty;
  }
}

/*initialize_robot initializes robot.*/
static void initialize_robot()
{
  /*Assign a position to the player.*/
  robot.x = randx();
  robot.y = randy();

  robot.character = '#';
  robot.color = ROBOT_COLOR;
  robot.bold = FALSE;
  screen[robot.x][robot.y] = ROBOT;
}

/*initialize kitten, well, initializes kitten.*/
static void initialize_kitten()
{
  /*Assign the kitten a unique position.*/
  do
    {
      kitten.x = randx();
      kitten.y = randy();
    } while (screen[kitten.x][kitten.y] != EMPTY);
  
  /*Assign the kitten a character and a color.*/
  do {
    kitten.character = randchar();
  } while (!(validchar(kitten.character))); 
  screen[kitten.x][kitten.y] = KITTEN;

  kitten.color = colors[randcolor()];
  kitten.bold = randbold();
}

/*initialize_bogus initializes all non-kitten objects to be used in this run.*/
static void initialize_bogus()
{
  int counter, index;
  for (counter = 0; counter < NUM_BOGUS; counter++)
    {
      /*Give it a color.*/
      bogus[counter].color = colors[randcolor()];
      bogus[counter].bold = randbold();
      
      /*Give it a character.*/
      do {
    bogus[counter].character = randchar();
      } while (!(validchar(bogus[counter].character))); 
      
      /*Give it a position.*/
      do
    {
      bogus[counter].x = randx();
      bogus[counter].y = randy();
    } while (screen[bogus[counter].x][bogus[counter].y] != EMPTY);

      screen[bogus[counter].x][bogus[counter].y] = counter+2;
      
      /*Find a message for this object.*/
      do {
        index = rb->rand() % MESSAGES;
      } while (used_messages[index] != false);
      bogus_messages[counter] = index;
      used_messages[index] = true;
    }

}

/*initialize_screen paints the screen.*/
static void initialize_screen()
{
  int counter;
  char buf[40];

  /*
   *Print the status portion of the screen.
   */
  rb->lcd_clear_display();
  rb->lcd_setfont(FONT_SYSFIXED);
  rb->snprintf(buf, sizeof(buf), "robotfindskitten %s", RFK_VERSION);
  rb->lcd_puts_scroll(0, 0, buf);
  refresh();
  
  /*Draw a line across the screen.*/
  for (counter = X_MIN; counter <= X_MAX + 1; counter++)
    {
      drawchar(counter, ADV_ROW+1, '_');
    }

  /*
   *Draw all the objects on the playing field.
   */
  for (counter = 0; counter < NUM_BOGUS; counter++)
    {
      draw(bogus[counter]);
    }

  draw(kitten);
  draw(robot);

  refresh();

}

/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
  (void)parameter;
  rb = api;

  exit_rfk = false;

  rb->srand(*rb->current_tick);

  initialize_arrays();

  /*
   * Now we initialize the various game objects.
   */
  initialize_robot();
  initialize_kitten();
  initialize_bogus();

  /*
   * Set up white-on-black screen on color targets
   */
#if LCD_DEPTH >= 16
  rb->lcd_set_backdrop(NULL);
  rb->lcd_set_foreground(LCD_WHITE);
  rb->lcd_set_background(LCD_BLACK);
#endif

  /*
   * Run the game
   */
  instructions();  

  initialize_screen();

  play_game();

  rb->lcd_setfont(FONT_UI);
  return PLUGIN_OK;
}
