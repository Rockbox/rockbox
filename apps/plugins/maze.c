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
#include "pluginlib_actions.h"
#include "helper.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   undef __PLUGINLIB_ACTIONS_H__
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

#endif

/* propertie bits of the cell */
#define WALL_N    0x00000001
#define WALL_E    0x00000002
#define WALL_S    0x00000004
#define WALL_W    0x00000008
#define WALL_ALL  0x0000000F

#define BORDER_N  0x00000010
#define BORDER_E  0x00000020
#define BORDER_S  0x00000040
#define BORDER_W  0x00000080
#define PATH      0x00000100

static const struct plugin_api* rb;

#ifdef __PLUGINLIB_ACTIONS_H__
const struct button_mapping *plugin_contexts[]
= {generic_directions, generic_actions};
#endif

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

struct coord_stack
{
    int data[MAZE_WIDTH*MAZE_HEIGHT];
    int stp;
};

void coord_stack_init(struct coord_stack* stack)
{
    stack->stp=0;
}

void coord_stack_push(struct coord_stack* stack, int x, int y)
{
    stack->data[stack->stp] = ((x<<8)|y);
    stack->stp++;
}

void coord_stack_get(struct coord_stack* stack, int index, int* x, int* y)
{
    *y = stack->data[index];
    *y &= 0xff;
    *x = (stack->data[index])>>8;
}

void coord_stack_pop(struct coord_stack* stack, int* x, int* y)
{
    stack->stp--;
    coord_stack_get(stack, stack->stp, x, y);
}

int coord_stack_count(struct coord_stack* stack)
{
    return(stack->stp);
}

struct maze
{
    int solved;
    int player_x;
    int player_y;
    unsigned short maze[MAZE_WIDTH][MAZE_HEIGHT];
};

void maze_init(struct maze* maze)
{
    int x, y;
    rb->memset(maze->maze, 0, sizeof(maze->maze));
    maze->solved = false;
    maze->player_x = 0;
    maze->player_y = 0;

    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){

            /* all walls are up */
            maze->maze[x][y] |= WALL_ALL | PATH;

            /* setup borders */
            if(x == 0)
                maze->maze[x][y]|= BORDER_W;
            if(y == 0)
                maze->maze[x][y]|= BORDER_N;
            if(x == MAZE_WIDTH-1)
                maze->maze[x][y]|= BORDER_E;
            if(y == MAZE_HEIGHT-1)
                maze->maze[x][y]|= BORDER_S;
        }
    }
}

void maze_draw(struct maze* maze, struct screen* display)
{
    int x, y;
    int wx, wy;
    int point_width, point_height, point_offset_x, point_offset_y;
    unsigned short cell;

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

    display->clear_display();

    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){

            cell = maze->maze[x][y];

            /* draw walls */
            if(cell & WALL_N)
                display->hline(x*wx, x*wx+wx, y*wy);
            if(cell & WALL_E)
                display->vline(x*wx+wx, y*wy, y*wy+wy);
            if(cell & WALL_S)
                display->hline(x*wx, x*wx+wx, y*wy+wy);
            if(cell & WALL_W)
                display->vline(x*wx, y*wy, y*wy+wy);

            if(cell & BORDER_N)
                display->hline(x*wx, x*wx+wx, y*wy);
            if(cell & BORDER_E)
                display->vline(x*wx+wx, y*wy, y*wy+wy);
            if(cell & BORDER_S)
                display->hline(x*wx, x*wx+wx, y*wy+wy);
            if(cell & BORDER_W)
                display->vline(x*wx, y*wy, y*wy+wy);
        }
    }
    if(maze->solved){
#if LCD_DEPTH >= 16
        if(display->depth>=16)
            display->set_foreground(LCD_RGBPACK(127,127,127));
#endif
#if LCD_DEPTH >= 2
        if(display->depth==2)
            display->set_foreground(1);
#endif
        for(y=0; y<MAZE_HEIGHT; y++){
            for(x=0; x<MAZE_WIDTH; x++){
                cell = maze->maze[x][y];
                if(cell & PATH)
                    display->fillrect(x*wx+point_offset_x,
                                      y*wy+point_offset_y,
                                      point_width, point_height);
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

    display->update();
}


int maze_pick_random_neighbour_cell_with_walls(struct maze* maze, 
                                     int x, int y, int *pnx, int *pny)
{

    int ncount = 0;
    struct coord_stack neighbours;
    unsigned short current_cell=maze->maze[x][y];

    coord_stack_init(&neighbours);
    /* look for neighbour cells with walls */

    /* north */
    if(!(current_cell & BORDER_N)){
        if((maze->maze[x][y-1] & WALL_ALL) == WALL_ALL){
            coord_stack_push(&neighbours, x, y-1);
        }
    }

    /* west */
    if(!(current_cell & BORDER_W)){
        if((maze->maze[x-1][y] & WALL_ALL) == WALL_ALL){
            coord_stack_push(&neighbours, x-1, y);
        }
    }

    /* east */
    if(!(current_cell & BORDER_E)){
        if((maze->maze[x+1][y] & WALL_ALL) == WALL_ALL){
            coord_stack_push(&neighbours, x+1, y);
        }
    }

    /* south */
    if(!(current_cell & BORDER_S)){
        if((maze->maze[x][y+1] & WALL_ALL) == WALL_ALL){
            coord_stack_push(&neighbours, x, y+1);
        }
    }

    /* randomly select neighbour cell with walls */
    ncount=coord_stack_count(&neighbours);
    if(ncount!=0)
        coord_stack_get(&neighbours, rb->rand()%ncount, pnx, pny);
    return ncount;
}

/* Removes the wall between the cell (x,y) and the cell (nx,ny) */
void maze_remove_wall(struct maze* maze, int x, int y, int nx, int ny)
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

void maze_generate(struct maze* maze)
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
            maze_remove_wall(maze, x, y, nx, ny);

            /* save position on the stack */
            coord_stack_push(&done_cells, x, y);

            /* current cell = neighbour cell */
            x=nx;
            y=ny;

            visited_cells++;
        }
    }
}


void maze_solve(struct maze* maze)
{
    int x, y;
    int dead_ends = 1;
    unsigned short cell;
    unsigned short  w;
    unsigned short solved_maze[MAZE_WIDTH][MAZE_HEIGHT];

    maze->solved = ~(maze->solved);

    /* copy maze for solving */
    rb->memcpy(solved_maze, maze->maze,
               (MAZE_HEIGHT*MAZE_WIDTH*sizeof(maze->maze[0][0])));


    /* remove some  borders and walls on start and end point */
    solved_maze[0][0] &= ~(WALL_N|BORDER_N);
    solved_maze[MAZE_WIDTH-1][MAZE_HEIGHT-1] &= ~(WALL_S|BORDER_S);

    while(dead_ends){
        dead_ends = 0;
        /* scan for dead ends */
        for(y=0; y<MAZE_HEIGHT; y++){
            rb->yield();
            for(x=0; x<MAZE_WIDTH; x++){
                cell = solved_maze[x][y];
                w = ~cell & 0x000f;
                if(w == WALL_N ||
                   w == WALL_E ||
                   w == WALL_S ||
                   w == WALL_W){
                    /* found dead end, clear path bit and fill it up */
                    maze->maze[x][y] &= ~PATH;
                    solved_maze[x][y] |= WALL_ALL;
                    /* don't forget the neighbours */
                    if(!(cell & BORDER_N))
                        solved_maze[x][y-1]|=WALL_S;
                    if(!(cell & BORDER_E))
                        solved_maze[x+1][y]|=WALL_W;
                    if(!(cell & BORDER_S))
                        solved_maze[x][y+1]|=WALL_N;
                    if(!(cell & BORDER_W))
                        solved_maze[x-1][y]|=WALL_E;
                    dead_ends++;
                }
            }
        }
    }
}

void maze_move_player_down(struct maze* maze)
{
    unsigned short cell=maze->maze[maze->player_x][maze->player_y];
    if( !(cell & WALL_S) &&
        !(cell & BORDER_S)){
        maze->player_y++;
    }
}

void maze_move_player_up(struct maze* maze)
{
    unsigned short cell=maze->maze[maze->player_x][maze->player_y];
    if( !(cell & WALL_N) &&
        !(cell & BORDER_N)){
        maze->player_y--;
    }
}

void maze_move_player_left(struct maze* maze)
{
    unsigned short cell=maze->maze[maze->player_x][maze->player_y];
    if( !(cell & WALL_W) &&
        !(cell & BORDER_W)){
        maze->player_x--;
    }
}

void maze_move_player_right(struct maze* maze)
{
    unsigned short cell=maze->maze[maze->player_x][maze->player_y];
    if( !(cell & WALL_E) &&
        !(cell & BORDER_E)){
        maze->player_x++;
    }
}
/**********************************/
/* this is the plugin entry point */
/**********************************/
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int button, lastbutton = BUTTON_NONE;
    int quit = 0;
    int i;
    struct maze maze;
    (void)parameter;
    rb = api;

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */
    
    FOR_NB_SCREENS(i)
        rb->screens[i]->set_viewport(NULL);
    
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#if LCD_DEPTH >= 16
    rb->lcd_set_foreground( LCD_RGBPACK( 0, 0, 0));
    rb->lcd_set_background(LCD_RGBPACK(182, 198, 229)); /* rockbox blue */
#elif LCD_DEPTH == 2
    rb->lcd_set_foreground(0);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
#endif
    maze_init(&maze);
    maze_generate(&maze);
    FOR_NB_SCREENS(i)
        maze_draw(&maze, rb->screens[i]);

    while(!quit) {
#ifdef __PLUGINLIB_ACTIONS_H__
        button = pluginlib_getaction(rb, TIMEOUT_BLOCK, plugin_contexts, 2);
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
        case MAZE_RIGHT:
        case MAZE_RRIGHT:
            maze_move_player_right(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_LEFT:
        case MAZE_RLEFT:
            maze_move_player_left(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_UP:
        case MAZE_RUP:
            maze_move_player_up(&maze);
            FOR_NB_SCREENS(i)
                maze_draw(&maze, rb->screens[i]);
            break;
        case MAZE_DOWN:
        case MAZE_RDOWN:
            maze_move_player_down(&maze);
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
        if(!quit)
            rb->yield(); /* BUG alert: always yielding causes data abort */
    }
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */
    return ((quit == 1) ? PLUGIN_OK : PLUGIN_USB_CONNECTED);
}
