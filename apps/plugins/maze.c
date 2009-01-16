/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Matthias Wientapper
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


/* This is the implementation of a maze generation algorithm.
 * The generated mazes are "perfect", i.e. there is one and only
 * one path from any point in the maze to any other point.
 *
 *
 * The implemented algorithm is called "Depth-First search", the
 * solving is done by a dead-end filler routine.
 *
 */

#include "plugin.h"
#include "lib/helper.h"

PLUGIN_HEADER

/* key assignments */

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   define MAZE_NEW      (BUTTON_SELECT | BUTTON_REPEAT)
#   define MAZE_NEW_PRE  BUTTON_SELECT
#   define MAZE_QUIT     (BUTTON_SELECT | BUTTON_MENU)
#   define MAZE_SOLVE    (BUTTON_SELECT | BUTTON_PLAY)
#   define MAZE_RIGHT    BUTTON_RIGHT
#   define MAZE_LEFT     BUTTON_LEFT
#   define MAZE_UP       BUTTON_MENU
#   define MAZE_DOWN     BUTTON_PLAY
#   define MAZE_RRIGHT   (BUTTON_RIGHT | BUTTON_REPEAT)
#   define MAZE_RLEFT    (BUTTON_LEFT | BUTTON_REPEAT)
#   define MAZE_RUP      (BUTTON_MENU | BUTTON_REPEAT)
#   define MAZE_RDOWN    (BUTTON_PLAY | BUTTON_REPEAT)

#else
#   include "lib/pluginlib_actions.h"
#   define MAZE_NEW      PLA_START
#   define MAZE_QUIT     PLA_QUIT
#   define MAZE_SOLVE    PLA_FIRE
#   define MAZE_RIGHT    PLA_RIGHT
#   define MAZE_LEFT     PLA_LEFT
#   define MAZE_UP       PLA_UP
#   define MAZE_DOWN     PLA_DOWN
#   define MAZE_RRIGHT   PLA_RIGHT_REPEAT
#   define MAZE_RLEFT    PLA_LEFT_REPEAT
#   define MAZE_RUP      PLA_UP_REPEAT
#   define MAZE_RDOWN    PLA_DOWN_REPEAT
static const struct button_mapping *plugin_contexts[]
= {generic_directions, generic_actions};

#endif

/* cell property bits */
#define WALL_N    0x0001
#define WALL_E    0x0002
#define WALL_S    0x0004
#define WALL_W    0x0008
#define WALL_ALL  (WALL_N | WALL_E | WALL_S | WALL_W)
#define PATH      0x0010

/* border tests */
#define BORDER_N(y) ((y) == 0)
#define BORDER_E(x) ((x) == MAZE_WIDTH-1)
#define BORDER_S(y) ((y) == MAZE_HEIGHT-1)
#define BORDER_W(x) ((x) == 0)

// we can and should change this to make square boxes
#if ( LCD_WIDTH == 112 )
#define MAZE_WIDTH  16
#define MAZE_HEIGHT 12
#elif( LCD_WIDTH == 132 )
#define MAZE_WIDTH  26
#define MAZE_HEIGHT 16
#else
#define MAZE_WIDTH  32
#define MAZE_HEIGHT 24
#endif

struct maze
{
    int show_path;
    int solved;
    int player_x;
    int player_y;
    uint8_t maze[MAZE_WIDTH][MAZE_HEIGHT];
};

static void maze_init(struct maze* maze)
{
    int x, y;

    /* initialize the properties */
    maze->show_path = false;
    maze->solved = false;
    maze->player_x = 0;
    maze->player_y = 0;

    /* all walls are up */
    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){
            maze->maze[x][y] = WALL_ALL;
        }
    }
}

static void maze_draw(struct maze* maze, struct screen* display)
{
    int x, y;
    int wx, wy;
    int point_width, point_height, point_offset_x, point_offset_y;
    uint8_t cell;

    /* calculate the size variables */

    wx = (int) display->lcdwidth / MAZE_WIDTH;
    wy = (int) display->lcdheight / MAZE_HEIGHT;

    if(wx>3){
        point_width=wx-3;
        point_offset_x=2;
    }else{
        point_width=1;
        point_offset_x=1;
    }

    if(wy>3){
        point_height=wy-3;
        point_offset_y=2;
    }else{
        point_height=1;
        point_offset_y=1;
    }

    /* start drawing */

    display->clear_display();

    /* draw the walls */
    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){
            cell = maze->maze[x][y];
            if(cell & WALL_N)
                display->hline(x*wx, x*wx+wx, y*wy);
            if(cell & WALL_E)
                display->vline(x*wx+wx, y*wy, y*wy+wy);
            if(cell & WALL_S)
                display->hline(x*wx, x*wx+wx, y*wy+wy);
            if(cell & WALL_W)
                display->vline(x*wx, y*wy, y*wy+wy);
        }
    }

    /* draw the path */
    if(maze->show_path){
#if LCD_DEPTH >= 16
        if(display->depth>=16)
            display->set_foreground(LCD_RGBPACK(127,127,127));
#endif
#if LCD_DEPTH >= 2
        if(display->depth==2)
            display->set_foreground(1);
#endif

        /* highlight the path */
        for(y=0; y<MAZE_HEIGHT; y++){
            for(x=0; x<MAZE_WIDTH; x++){
                cell = maze->maze[x][y];
                if(cell & PATH)
                    display->fillrect(x*wx+point_offset_x,
                                      y*wy+point_offset_y,
                                      point_width, point_height);
            }
        }

        /* link the cells in the path together */
        for(y=0; y<MAZE_HEIGHT; y++){
            for(x=0; x<MAZE_WIDTH; x++){
                cell = maze->maze[x][y];
                if(cell & PATH){
                    if(!(cell & WALL_N) && (maze->maze[x][y-1] & PATH))
                        display->fillrect(x*wx+point_offset_x,
                                          y*wy,
                                          point_width, wy-point_height);
                    if(!(cell & WALL_E) && (maze->maze[x+1][y] & PATH))
                        display->fillrect(x*wx+wx-point_offset_x,
                                          y*wy+point_offset_y,
                                          wx-point_width, point_height);
                    if(!(cell & WALL_S) && (maze->maze[x][y+1] & PATH))
                        display->fillrect(x*wx+point_offset_x,
                                          y*wy+wy-point_offset_y,
                                          point_width, wy-point_height);
                    if(!(cell & WALL_W) && (maze->maze[x-1][y] & PATH))
                        display->fillrect(x*wx,
                                          y*wy+point_offset_y,
                                          wx-point_width, point_height);
                }
            }
        }

#if LCD_DEPTH >= 16
        if(display->depth>=16)
            display->set_foreground(LCD_RGBPACK(0,0,0));
#endif
#if LCD_DEPTH >= 2
        if(display->depth==2)
            display->set_foreground(0);
#endif
    }

    /* mark start and end */
    display->drawline(0, 0, wx, wy);
    display->drawline(0, wy, wx, 0);
    display->drawline((MAZE_WIDTH-1)*wx,(MAZE_HEIGHT-1)*wy,
                     (MAZE_WIDTH-1)*wx+wx, (MAZE_HEIGHT-1)*wy+wy);
    display->drawline((MAZE_WIDTH-1)*wx,(MAZE_HEIGHT-1)*wy+wy,
                     (MAZE_WIDTH-1)*wx+wx, (MAZE_HEIGHT-1)*wy);

    /* draw current position */
    display->fillrect(maze->player_x*wx+point_offset_x,
                      maze->player_y*wy+point_offset_y,
                      point_width, point_height);

    /* update the display */
    display->update();
}


struct coord_stack
{
    uint8_t x[MAZE_WIDTH*MAZE_HEIGHT];
    uint8_t y[MAZE_WIDTH*MAZE_HEIGHT];
    int stp;
};

static void coord_stack_init(struct coord_stack* stack)
{
    rb->memset(stack->x, 0, sizeof(stack->x));
    rb->memset(stack->y, 0, sizeof(stack->y));
    stack->stp = 0;
}

static void coord_stack_push(struct coord_stack* stack, int x, int y)
{
    stack->x[stack->stp] = x;
    stack->y[stack->stp] = y;
    stack->stp++;
}

static void coord_stack_pop(struct coord_stack* stack, int* x, int* y)
{
    stack->stp--;
    *x = stack->x[stack->stp];
    *y = stack->y[stack->stp];
}

static int maze_pick_random_neighbour_cell_with_walls(struct maze* maze, 
                                     int x, int y, int *pnx, int *pny)
{
    int n, i;
    int px[4], py[4];

    n = 0;

    /* look for neighbours with all walls set up */

    if(!BORDER_N(y) && ((maze->maze[x][y-1] & WALL_ALL) == WALL_ALL)){
        px[n] = x;
        py[n] = y-1;
        n++;
    }

    if(!BORDER_E(x) && ((maze->maze[x+1][y] & WALL_ALL) == WALL_ALL)){
        px[n] = x+1;
        py[n] = y;
        n++;
    }

    if(!BORDER_S(y) && ((maze->maze[x][y+1] & WALL_ALL) == WALL_ALL)){
        px[n] = x;
        py[n] = y+1;
        n++;
    }

    if(!BORDER_W(x) && ((maze->maze[x-1][y] & WALL_ALL) == WALL_ALL)){
        px[n] = x-1;
        py[n] = y;
        n++;
    }

    /* then choose one */
    if (n > 0){
        i = rb->rand() % n;
        *pnx = px[i];
        *pny = py[i];
    }

    return n;
}

/* Removes the wall between the cell (x,y) and the cell (nx,ny) */
static void maze_remove_wall(struct maze* maze, int x, int y, int nx, int ny)
{
    /* where is our neighbour? */

    /* north or south */
    if(x==nx){
        if(y<ny){
            /*south*/
            maze->maze[x][y] &= ~WALL_S;
            maze->maze[nx][ny] &= ~WALL_N;
        } else {
            /*north*/
            maze->maze[x][y] &= ~WALL_N;
            maze->maze[nx][ny] &= ~WALL_S;
        }
    } else {
        /* east or west */
        if(y==ny){
            if(x<nx){
                /* east */
                maze->maze[x][y] &= ~WALL_E;
                maze->maze[nx][ny] &= ~WALL_W;
            } else {
                /*west*/
                maze->maze[x][y] &= ~WALL_W;
                maze->maze[nx][ny] &= ~WALL_E;
            }
        }
    }
}

static void maze_generate(struct maze* maze)
{
    int total_cells = MAZE_WIDTH * MAZE_HEIGHT;
    int visited_cells;
    int available_neighbours;
    int x, y;
    int nx = 0;
    int ny = 0;
    struct coord_stack done_cells;

    coord_stack_init(&done_cells);

    x = rb->rand()%MAZE_WIDTH;
    y = rb->rand()%MAZE_HEIGHT;

    visited_cells = 1;
    while (visited_cells < total_cells){
        available_neighbours =
            maze_pick_random_neighbour_cell_with_walls(maze, x, y, &nx, &ny);
        if(available_neighbours == 0){
            /* pop from stack */
            coord_stack_pop(&done_cells, &x, &y);
        } else {
            /* remove the wall */
            maze_remove_wall(maze, x, y, nx, ny);
            /* save our position on the stack */
            coord_stack_push(&done_cells, x, y);
            /* move to the next cell */
            x=nx;
            y=ny;
            /* keep track of visited cells count */
            visited_cells++;
        }
    }
}


static void maze_solve(struct maze* maze)
{
    int x, y;
    int dead_ends = 1;
    uint8_t cell;
    uint8_t wall;
    uint8_t solved_maze[MAZE_WIDTH][MAZE_HEIGHT];

    /* toggle the visibility of the path */
    maze->show_path = ~(maze->show_path);

    /* no need to solve the maze if already solved */
    if (maze->solved)
        return;

    /* work on a copy of the maze */
    rb->memcpy(solved_maze, maze->maze, sizeof(maze->maze));

    /* remove walls on start and end point */
    solved_maze[0][0] &= ~WALL_N;
    solved_maze[MAZE_WIDTH-1][MAZE_HEIGHT-1] &= ~WALL_S;

    /* first, mark all the cells as reachable */
    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){
            solved_maze[x][y] |= PATH;
        }
    }

    /* start solving */
    while(dead_ends){
        /* solve by blocking off dead ends -- backward approach */
        dead_ends = 0;
        /* scan for dead ends */
        for(y=0; y<MAZE_HEIGHT; y++){
            rb->yield();
            for(x=0; x<MAZE_WIDTH; x++){
                cell = solved_maze[x][y];
                wall = cell & WALL_ALL;
                if((wall == (WALL_E | WALL_S | WALL_W)) ||
                   (wall == (WALL_N | WALL_S | WALL_W)) ||
                   (wall == (WALL_N | WALL_E | WALL_W)) ||
                   (wall == (WALL_N | WALL_E | WALL_S))){
                    /* found dead end, clear path bit and set all its walls */
                    solved_maze[x][y] &= ~PATH;
                    solved_maze[x][y] |= WALL_ALL;
                    /* don't forget the neighbours */
                    if(!BORDER_S(y))
                        solved_maze[x][y+1] |= WALL_N;
                    if(!BORDER_W(x))
                        solved_maze[x-1][y] |= WALL_E;
                    if(!BORDER_N(y))
                        solved_maze[x][y-1] |= WALL_S;
                    if(!BORDER_E(x))
                        solved_maze[x+1][y] |= WALL_W;
                    dead_ends++;
                }
            }
        }
    }

    /* copy all the path bits to the maze */
    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){
            maze->maze[x][y] |= solved_maze[x][y] & PATH;
        }
    }

    /* mark the maze as solved */
    maze->solved = true;
}

static void maze_move_player_up(struct maze* maze)
{
    uint8_t cell = maze->maze[maze->player_x][maze->player_y];
    if(!BORDER_N(maze->player_y) && !(cell & WALL_N))
        maze->player_y--;
}

static void maze_move_player_right(struct maze* maze)
{
    uint8_t cell = maze->maze[maze->player_x][maze->player_y];
    if(!BORDER_E(maze->player_x) && !(cell & WALL_E))
        maze->player_x++;
}

static void maze_move_player_down(struct maze* maze)
{
    uint8_t cell = maze->maze[maze->player_x][maze->player_y];
    if(!BORDER_S(maze->player_y) && !(cell & WALL_S))
        maze->player_y++;
}

static void maze_move_player_left(struct maze* maze)
{
    uint8_t cell = maze->maze[maze->player_x][maze->player_y];
    if(!BORDER_W(maze->player_x) && !(cell & WALL_W))
        maze->player_x--;
}

/**********************************/
/* this is the plugin entry point */
/**********************************/
enum plugin_status plugin_start(const void* parameter)
{
    int button, lastbutton = BUTTON_NONE;
    int quit = 0;
    int i;
    struct maze maze;
    (void)parameter;

    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    /* Seed the RNG */
    rb->srand(*rb->current_tick);

    FOR_NB_SCREENS(i)
        rb->screens[i]->set_viewport(NULL);

    /* Draw the background */
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#if LCD_DEPTH >= 16
    rb->lcd_set_foreground(LCD_RGBPACK( 0, 0, 0));
    rb->lcd_set_background(LCD_RGBPACK(182, 198, 229)); /* rockbox blue */
#elif LCD_DEPTH == 2
    rb->lcd_set_foreground(0);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
#endif

    /* Initialize and draw the maze */
    maze_init(&maze);
    maze_generate(&maze);
    FOR_NB_SCREENS(i)
        maze_draw(&maze, rb->screens[i]);

    while(!quit) {
#ifdef __PLUGINLIB_ACTIONS_H__
        button = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts, 2);
#else
        button = rb->button_get(true);
#endif
        switch(button) {
        case MAZE_NEW:
#ifdef MAZE_NEW_PRE
            if(lastbutton != MAZE_NEW_PRE)
                break;
#endif
            maze_init(&maze);
            maze_generate(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_SOLVE:
            maze_solve(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_UP:
        case MAZE_RUP:
            maze_move_player_up(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_RIGHT:
        case MAZE_RRIGHT:
            maze_move_player_right(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_DOWN:
        case MAZE_RDOWN:
            maze_move_player_down(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_LEFT:
        case MAZE_RLEFT:
            maze_move_player_left(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_QUIT:
            /* quit plugin */
            quit=1;
            break;
        default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                /* quit plugin */
                quit=2;
            }
            break;
        }
        if( button != BUTTON_NONE )
            lastbutton = button;

    }
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */
    return ((quit == 1) ? PLUGIN_OK : PLUGIN_USB_CONNECTED);
}
