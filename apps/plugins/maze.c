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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

PLUGIN_HEADER

#define MAZE_NEW   PLA_START
#define MAZE_QUIT  PLA_QUIT
#define MAZE_SOLVE PLA_FIRE

#define MAZE_RIGHT PLA_RIGHT
#define MAZE_LEFT  PLA_LEFT
#define MAZE_UP    PLA_UP
#define MAZE_DOWN  PLA_DOWN
#define MAZE_RRIGHT PLA_RIGHT_REPEAT
#define MAZE_RLEFT  PLA_LEFT_REPEAT
#define MAZE_RUP    PLA_UP_REPEAT
#define MAZE_RDOWN  PLA_DOWN_REPEAT


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

static struct plugin_api* rb;
const struct button_mapping *plugin_contexts[]
= {generic_directions, generic_actions};

#if ( LCD_WIDTH == 112 )
#define MAZE_WIDTH  16
#define MAZE_HEIGHT 12
#else
#define MAZE_WIDTH  32
#define MAZE_HEIGHT 24
#endif

unsigned short maze[MAZE_WIDTH][MAZE_HEIGHT];
unsigned short solved_maze[MAZE_WIDTH][MAZE_HEIGHT];

int stack[MAZE_WIDTH*MAZE_HEIGHT];
int solved = false;
char buf[30];

int sy = 0;
int sx = 0;


void init_maze(void){
    int x, y;

    rb->memset(maze, 0, sizeof(maze));
    sx = 0;
    sy = 0;

    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){

            /* all walls are up */
            maze[x][y] |= WALL_ALL | PATH;

            /* setup borders */
            if(x == 0)
                maze[x][y]|= BORDER_W;
            if(y == 0)
                maze[x][y]|= BORDER_N;
            if(x == MAZE_WIDTH-1)
                maze[x][y]|= BORDER_E;
            if(y == MAZE_HEIGHT-1)
                maze[x][y]|= BORDER_S;
        }
    }
}

void show_maze(void){
    int x, y;
    int wx, wy;
    unsigned short cell;

    wx = (int) LCD_WIDTH / MAZE_WIDTH;
    wy = (int) LCD_HEIGHT / MAZE_HEIGHT;

    rb->lcd_clear_display();

    for(y=0; y<MAZE_HEIGHT; y++){
        for(x=0; x<MAZE_WIDTH; x++){

            cell = maze[x][y];

            /* draw walls */
            if(cell & WALL_N)
                rb->lcd_drawline(x*wx, y*wy, x*wx+wx, y*wy);
            if(cell & WALL_E)
                rb->lcd_drawline(x*wx+wx, y*wy, x*wx+wx, y*wy+wy);
            if(cell & WALL_S)
                rb->lcd_drawline(x*wx, y*wy+wy, x*wx+wx, y*wy+wy);
            if(cell & WALL_W)
                rb->lcd_drawline(x*wx, y*wy, x*wx, y*wy+wy);

            if(cell & BORDER_N)
                rb->lcd_drawline(x*wx, y*wy, x*wx+wx, y*wy);
            if(cell & BORDER_E)
                rb->lcd_drawline(x*wx+wx, y*wy, x*wx+wx, y*wy+wy);
            if(cell & BORDER_S)
                rb->lcd_drawline(x*wx, y*wy+wy, x*wx+wx, y*wy+wy);
            if(cell & BORDER_W)
                rb->lcd_drawline(x*wx, y*wy, x*wx, y*wy+wy);

            if(solved){
                if(cell & PATH){
#if LCD_DEPTH >= 16
                    rb->lcd_set_foreground( LCD_RGBPACK( 127, 127, 127 ));
#elif LCD_DEPTH == 2
                    rb->lcd_set_foreground(1);
#endif
                    rb->lcd_fillrect(x*wx+2, y*wy+2, wx-3, wy-3);
#if LCD_DEPTH >= 16
                    rb->lcd_set_foreground( LCD_RGBPACK( 0, 0, 0));
#elif LCD_DEPTH == 2
                    rb->lcd_set_foreground(0);
#endif
                }
            }
        }
    }

    /* mark start and end */
    rb->lcd_drawline(0, 0, wx, wy);
    rb->lcd_drawline(0, wy, wx, 0);
    rb->lcd_drawline((MAZE_WIDTH-1)*wx,(MAZE_HEIGHT-1)*wy,
                     (MAZE_WIDTH-1)*wx+wx, (MAZE_HEIGHT-1)*wy+wy);
    rb->lcd_drawline((MAZE_WIDTH-1)*wx,(MAZE_HEIGHT-1)*wy+wy,
                     (MAZE_WIDTH-1)*wx+wx, (MAZE_HEIGHT-1)*wy);


    /* draw current position */
    rb->lcd_fillrect(sx*wx+2, sy*wy+2, wx-3, wy-3);

    rb->lcd_update();
}


int random_neighbour_cell_with_walls(int *px, int *py, int *pnx, int *pny){

    int ncount = 0;
    int neighbours[4];
    int found_cell;


    /* look for neighbour cells with walls */

    /* north */
    if(!(maze[*px][*py] & BORDER_N)){
        if((maze[*px][*py-1] & WALL_ALL) == WALL_ALL){
            /* save found cell coordinates */
            neighbours[ncount]=(*px<<8)|((*py)-1);
            ncount++;
        }
    }

    /* west */
    if(!(maze[*px][*py] & BORDER_W)){
        if((maze[*px-1][*py] & WALL_ALL) == WALL_ALL){
            /* save found cell coordinates */
            neighbours[ncount]=((*px-1)<<8)|(*py);
            ncount++;
        }
    }

    /* east */
    if(!(maze[*px][*py] & BORDER_E)){
        if((maze[*px+1][*py] & WALL_ALL) == WALL_ALL){
            /* save found cell coordinates */
            neighbours[ncount]=((*px+1)<<8)|(*py);
            ncount++;
        }
    }

    /* south */
    if(!(maze[*px][*py] & BORDER_S)){
        if((maze[*px][*py+1] & WALL_ALL) == WALL_ALL){
            /* save found cell coordinates */
            neighbours[ncount]=(*px<<8)|((*py)+1);
            ncount++;
        }
    }

    /* randomly select neighbour cell with walls */
    if(ncount!=0){
        found_cell = neighbours[rb->rand()%ncount];
        *pny = found_cell &0x000000ff;
        *pnx = (unsigned int) found_cell >> 8;
    }
    return ncount;
}

void remove_walls(int *px, int *py, int *pnx, int *pny){

    /* where is our neighbour? */

    /* north or south */
    if(*px==*pnx){
        if(*py<*pny){
            /*south*/
            maze[*px][*py] &= ~WALL_S;
            maze[*pnx][*pny] &= ~WALL_N;
        } else {
            /*north*/
            maze[*px][*py] &= ~WALL_N;
            maze[*pnx][*pny] &= ~WALL_S;
        }
    } else {
        /* east or west */
        if(*py==*pny){
            if(*px<*pnx){
                /* east */
                maze[*px][*py] &= ~WALL_E;
                maze[*pnx][*pny] &= ~WALL_W;

            } else {
                /*west*/
                maze[*px][*py] &= ~WALL_W;
                maze[*pnx][*pny] &= ~WALL_E;
            }
        }
    }
}


void generate_maze(void){
    int stp = 0;
    int total_cells = MAZE_WIDTH * MAZE_HEIGHT;
    int visited_cells;
    int neighbour_cell;
    int x, y;
    int nx = 0;
    int ny = 0;
    int *px, *py, *pnx, *pny;

    px = &x;
    py = &y;
    pnx = &nx;
    pny = &ny;

    x = rb->rand()%MAZE_WIDTH;
    y = rb->rand()%MAZE_HEIGHT;

    visited_cells = 1;
    while (visited_cells < total_cells){
        neighbour_cell = random_neighbour_cell_with_walls(px, py, pnx, pny);
        if(neighbour_cell == 0){

            /* pop from stack */
            stp--;
            *py = stack[stp];
            *py &= 0xff;
            *px = (stack[stp])>>8;
        } else {
            remove_walls(px, py, pnx, pny);

            /* save position on the stack */
            stack[stp] = ((*px<<8)|*py);
            stp++;
            /* current cell = neighbour cell */
            x=nx;
            y=ny;

            visited_cells++;
        }
    }
}


void solve_maze(void){
    int x, y;
    unsigned short cell;
    unsigned short  w;
    int dead_ends = 1;

    /* dead end filler*/

    /* copy maze for solving */
    rb->memcpy(solved_maze, maze, (MAZE_HEIGHT*MAZE_WIDTH*sizeof(maze[0][0])));


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
                    maze[x][y] &= ~PATH;
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

/**********************************/
/* this is the plugin entry point */
/**********************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button = 0;
    int quit = 0;

    (void)parameter;
    rb = api;

    rb->backlight_set_timeout(1);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif

    init_maze();
    generate_maze();
    show_maze();

    while(!quit) {
        button = pluginlib_getaction(rb, TIMEOUT_BLOCK, plugin_contexts, 2);
        switch(button) {
        case MAZE_NEW:
            solved = false;
            init_maze();
            generate_maze();
            show_maze();
            break;
        case MAZE_SOLVE:
            solved = ~solved;
            solve_maze();
            show_maze();
            break;
        case MAZE_RIGHT:
        case MAZE_RRIGHT:
            if( !(maze[sx][sy] & WALL_E) && !(maze[sx][sy] & BORDER_E)){
                sx++;
                show_maze();
            }
            break;
        case MAZE_LEFT:
        case MAZE_RLEFT:
            if( !(maze[sx][sy] & WALL_W) && !(maze[sx][sy] & BORDER_W)){
                sx--;
                show_maze();
            }
            break;
        case MAZE_UP:
        case MAZE_RUP:
            if( !(maze[sx][sy] & WALL_N) && !(maze[sx][sy] & BORDER_N)){
                sy--;
                show_maze();
            }
            break;
        case MAZE_DOWN:
        case MAZE_RDOWN:
            if( !(maze[sx][sy] & WALL_S) && !(maze[sx][sy] & BORDER_S)){
                sy++;
                show_maze();
            }
            break;

        case MAZE_QUIT:
            /* quit plugin */
            quit=true;
            return PLUGIN_OK;
            break;
        default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                return PLUGIN_USB_CONNECTED;
            }
            break;
        }
        rb->yield();
    }
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
    return PLUGIN_OK;
}
