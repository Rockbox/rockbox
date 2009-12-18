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

/*
 * This is an implementatino of Conway's Game of Life
 *
 * from http://en.wikipedia.org/wiki/Conway's_Game_of_Life:
 *
 * Rules
 *
 * The universe of the Game of Life is an infinite two-dimensional
 * orthogonal grid of square cells, each of which is in one of two
 * possible states, live or dead. Every cell interacts with its eight
 * neighbours, which are the cells that are directly horizontally,
 * vertically, or diagonally adjacent. At each step in time, the
 * following transitions occur:
 *
 * 1. Any live cell with fewer than two live neighbours dies, as if by
 *    loneliness.
 *
 * 2. Any live cell with more than three live neighbours dies, as if
 *    by overcrowding.
 *
 * 3. Any live cell with two or three live neighbours lives,
 *    unchanged, to the next generation.
 *
 * 4. Any dead cell with exactly three live neighbours comes to life.
 *
 * The initial pattern constitutes the first generation of the
 * system. The second generation is created by applying the above
 * rules simultaneously to every cell in the first generation --
 * births and deaths happen simultaneously, and the discrete moment at
 * which this happens is sometimes called a tick. (In other words,
 * each generation is based entirely on the one before.) The rules
 * continue to be applied repeatedly to create further generations.
 *
 *
 *
 * TODO:
 *       - nicer colours for pixels with respect to age
 *       - editor for start patterns
 *       - probably tons of speed-up opportunities
 */

#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/helper.h"

PLUGIN_HEADER

#define ROCKLIFE_PLAY_PAUSE PLA_FIRE
#define ROCKLIFE_INIT       PLA_DOWN
#define ROCKLIFE_NEXT       PLA_RIGHT
#define ROCKLIFE_NEXT_REP   PLA_RIGHT_REPEAT
#define ROCKLIFE_QUIT       PLA_QUIT
#define ROCKLIFE_STATUS     PLA_LEFT

#define PATTERN_RANDOM     0
#define PATTERN_GROWTH_1   1
#define PATTERN_GROWTH_2   2
#define PATTERN_ACORN      3
#define PATTERN_GLIDER_GUN 4

const struct button_mapping *plugin_contexts[]
= {generic_directions, generic_actions};

#define GRID_W LCD_WIDTH
#define GRID_H LCD_HEIGHT

unsigned char grid_a[GRID_W][GRID_H];
unsigned char grid_b[GRID_W][GRID_H];
int generation = 0;
int population = 0;
int status_line = 0;
char buf[30];


static inline bool is_valid_cell(int x, int y) {
    return (x >= 0 && x < GRID_W
            && y >= 0 && y < GRID_H);
}

static inline void set_cell_age(int x, int y, unsigned char age, char *pgrid) {
    pgrid[x+y*GRID_W] = age;
}

static inline void set_cell(int x, int y, char *pgrid) {
    set_cell_age(x, y, 1, pgrid);
}

static inline unsigned char get_cell(int x, int y, char *pgrid) {
    if (x < 0)
        x += GRID_W;
    else if (x >= GRID_W)
        x -= GRID_W;

    if (y < 0)
        y += GRID_H;
    else if (y >= GRID_H)
        y -= GRID_H;

    return pgrid[x+y*GRID_W];
}

/* clear grid */
void init_grid(char *pgrid){
    memset(pgrid, 0, GRID_W * GRID_H);
}

/*fill grid with pattern from file (viewer mode)*/
static bool load_cellfile(const char *file, char *pgrid){
    int fd;
    fd = rb->open(file, O_RDONLY);
    if (fd<0)
        return false;

    init_grid(pgrid);

    char c;
    int nc, x, y, xmid, ymid;
    bool comment;
    x=0;
    y=0;
    xmid = (GRID_W>>1) - 2;
    ymid = (GRID_H>>1) - 2;
    comment = false;

    while (true) {
        nc = rb->read(fd, &c, 1);
        if (nc <= 0)
            break;

        switch(c) {
        case '!':
            comment = true;
        case '.':
            if (!comment)
                x++;
            break;
        case 'O':
            if (!comment) {
                if (is_valid_cell(xmid + x, ymid + y))
                    set_cell(xmid + x, ymid + y, pgrid);
                x++;
            }
            break;
        case '\n':
            y++;
            x=0;
            comment = false;
            break;
        default:
            break;
        }
    }
    rb->close(fd);
    return true;
}

/* fill grid with initial pattern */
static void setup_grid(char *pgrid, int pattern){
    int n, max;
    int xmid, ymid;

    max = GRID_W * GRID_H;

    switch(pattern){
    case PATTERN_RANDOM:
        rb->splash(HZ, "Random");
#if 0 /* two oscilators, debug pattern */
        set_cell( 0,  1 , pgrid);
        set_cell( 1,  1 , pgrid);
        set_cell( 2,  1 , pgrid);

        set_cell( 6,  7 , pgrid);
        set_cell( 7,  7 , pgrid);
        set_cell( 8,  7 , pgrid);
#endif

        /* fill screen randomly */
        for(n=0; n<(max>>2); n++)
            pgrid[rb->rand()%max] = 1;

        break;

    case PATTERN_GROWTH_1:
        rb->splash(HZ, "Growth");
        xmid = (GRID_W>>1) - 2;
        ymid = (GRID_H>>1) - 2;
        set_cell(xmid + 6, ymid + 0 , pgrid);
        set_cell(xmid + 4, ymid + 1 , pgrid);
        set_cell(xmid + 6, ymid + 1 , pgrid);
        set_cell(xmid + 7, ymid + 1 , pgrid);
        set_cell(xmid + 4, ymid + 2 , pgrid);
        set_cell(xmid + 6, ymid + 2 , pgrid);
        set_cell(xmid + 4, ymid + 3 , pgrid);
        set_cell(xmid + 2, ymid + 4 , pgrid);
        set_cell(xmid + 0, ymid + 5 , pgrid);
        set_cell(xmid + 2, ymid + 5 , pgrid);
        break;
    case PATTERN_ACORN:
        rb->splash(HZ, "Acorn");
        xmid = (GRID_W>>1) - 3;
        ymid = (GRID_H>>1) - 1;
        set_cell(xmid + 1, ymid + 0 , pgrid);
        set_cell(xmid + 3, ymid + 1 , pgrid);
        set_cell(xmid + 0, ymid + 2 , pgrid);
        set_cell(xmid + 1, ymid + 2 , pgrid);
        set_cell(xmid + 4, ymid + 2 , pgrid);
        set_cell(xmid + 5, ymid + 2 , pgrid);
        set_cell(xmid + 6, ymid + 2 , pgrid);
        break;
    case PATTERN_GROWTH_2:
        rb->splash(HZ, "Growth 2");
        xmid = (GRID_W>>1) - 4;
        ymid = (GRID_H>>1) - 1;
        set_cell(xmid + 0, ymid + 0 , pgrid);
        set_cell(xmid + 1, ymid + 0 , pgrid);
        set_cell(xmid + 2, ymid + 0 , pgrid);
        set_cell(xmid + 4, ymid + 0 , pgrid);
        set_cell(xmid + 0, ymid + 1 , pgrid);
        set_cell(xmid + 3, ymid + 2 , pgrid);
        set_cell(xmid + 4, ymid + 2 , pgrid);
        set_cell(xmid + 1, ymid + 3 , pgrid);
        set_cell(xmid + 2, ymid + 3 , pgrid);
        set_cell(xmid + 4, ymid + 3 , pgrid);
        set_cell(xmid + 0, ymid + 4 , pgrid);
        set_cell(xmid + 2, ymid + 4 , pgrid);
        set_cell(xmid + 4, ymid + 4 , pgrid);
        break;
    case PATTERN_GLIDER_GUN:
        rb->splash(HZ, "Glider Gun");
        set_cell( 24, 0, pgrid);
        set_cell( 22, 1, pgrid);
        set_cell( 24, 1, pgrid);
        set_cell( 12, 2, pgrid);
        set_cell( 13, 2, pgrid);
        set_cell( 20, 2, pgrid);
        set_cell( 21, 2, pgrid);
        set_cell( 34, 2, pgrid);
        set_cell( 35, 2, pgrid);
        set_cell( 11, 3, pgrid);
        set_cell( 15, 3, pgrid);
        set_cell( 20, 3, pgrid);
        set_cell( 21, 3, pgrid);
        set_cell( 34, 3, pgrid);
        set_cell( 35, 3, pgrid);
        set_cell(  0, 4, pgrid);
        set_cell(  1, 4, pgrid);
        set_cell( 10, 4, pgrid);
        set_cell( 16, 4, pgrid);
        set_cell( 20, 4, pgrid);
        set_cell( 21, 4, pgrid);
        set_cell(  0, 5, pgrid);
        set_cell(  1, 5, pgrid);
        set_cell( 10, 5, pgrid);
        set_cell( 14, 5, pgrid);
        set_cell( 16, 5, pgrid);
        set_cell( 17, 5, pgrid);
        set_cell( 22, 5, pgrid);
        set_cell( 24, 5, pgrid);
        set_cell( 10, 6, pgrid);
        set_cell( 16, 6, pgrid);
        set_cell( 24, 6, pgrid);
        set_cell( 11, 7, pgrid);
        set_cell( 15, 7, pgrid);
        set_cell( 12, 8, pgrid);
        set_cell( 13, 8, pgrid);
        break;
    }
}

/* display grid */
static void show_grid(char *pgrid){
    int x, y;
    unsigned char age;

    rb->lcd_clear_display();
    for(y=0; y<GRID_H; y++){
        for(x=0; x<GRID_W; x++){
            age = get_cell(x, y, pgrid);
            if(age){
#if LCD_DEPTH >= 16
                rb->lcd_set_foreground( LCD_RGBPACK( age, age, age ));
#elif LCD_DEPTH == 2
                rb->lcd_set_foreground(age>>7);
#endif
                rb->lcd_drawpixel(x, y);
            }
        }
    }
    if(status_line){
        rb->snprintf(buf, sizeof(buf), "g:%d p:%d", generation, population);
#if LCD_DEPTH > 1
        rb->lcd_set_foreground( LCD_BLACK );
#endif
        rb->lcd_puts(0, 0, buf);
    }
    rb->lcd_update();
}


/* Calculates whether the cell will be alive in the next generation.
   n is the array with 9 elements that represent the cell itself and its
   neighborhood like this (the cell itself is n[4]):
   0 1 2
   3 4 5
   6 7 8
*/
static inline bool check_cell(unsigned char *n)
{
    int empty_cells = 0;
    int alive_cells;
    bool result;

    /* count empty neighbour cells */
    if(n[0]==0) empty_cells++;
    if(n[1]==0) empty_cells++;
    if(n[2]==0) empty_cells++;
    if(n[3]==0) empty_cells++;
    if(n[5]==0) empty_cells++;
    if(n[6]==0) empty_cells++;
    if(n[7]==0) empty_cells++;
    if(n[8]==0) empty_cells++;

    /* now we build the number of non-zero neighbours :-P */
    alive_cells = 8 - empty_cells;

    if (n[4]) {
        /* If the cell is alive, it stays alive iff it has 2 or 3 alive neighbours */
        result = (alive_cells==2 || alive_cells==3);
    }
    else {
        /* If the cell is dead, it gets alive iff it has 3 alive neighbours */
        result = (alive_cells==3);
    }

    return result;
}

/* Calculate the next generation of cells
 *
 * The borders of the grid are connected to their opposite sides.
 *
 * To avoid multiplications while accessing data in the 2-d grid
 * (pgrid) we try to re-use previously accessed neighbourhood
 * information which is stored in an 3x3 array.
 */
static void next_generation(char *pgrid, char *pnext_grid){
    int x, y;
    bool cell;
    unsigned char age;
    unsigned char n[9];

    rb->memset(n, 0, sizeof(n));

    /*
     * cell is (4) with 8 neighbours
     *
     *   0|1|2
     *   -----
     *   3|4|5
     *   -----
     *   6|7|8
     */

    population = 0;

    /* go through the grid */
    for(y=0; y<GRID_H; y++){
        for(x=0; x<GRID_W; x++){
            if(y==0 && x==0){
                /* first cell in first row, we have to load all neighbours */
                n[0] = get_cell(x-1, y-1, pgrid);
                n[1] = get_cell(x,   y-1, pgrid);
                n[2] = get_cell(x+1, y-1, pgrid);
                n[3] = get_cell(x-1, y,   pgrid);
                n[4] = get_cell(x,   y,   pgrid);
                n[5] = get_cell(x+1, y,   pgrid);
                n[6] = get_cell(x-1, y+1, pgrid);
                n[7] = get_cell(x,   y+1, pgrid);
                n[8] = get_cell(x+1, y+1, pgrid);
            } else {
                if(x==0){
                    /* beginning of a row, copy what we know about our predecessor,
                       0, 1, 3, 4 are known, 2, 5, 6, 7, 8 have to be loaded
                    */
                    n[0] = n[4];
                    n[1] = n[5];
                    n[2] = get_cell(x+1, y-1, pgrid);
                    n[3] = n[7];
                    n[4] = n[8];
                    n[5] = get_cell(x+1, y,   pgrid);
                    n[6] = get_cell(x-1, y+1, pgrid);
                    n[7] = get_cell(x,   y+1, pgrid);
                    n[8] = get_cell(x+1, y+1, pgrid);
                } else {
                    /* we are moving right in a row,
                     * copy what we know about the neighbours on our left side,
                     * 2, 5, 8 have to be loaded
                     */
                    n[0] = n[1];
                    n[1] = n[2];
                    n[2] = get_cell(x+1, y-1, pgrid);
                    n[3] = n[4];
                    n[4] = n[5];
                    n[5] = get_cell(x+1, y,   pgrid);
                    n[6] = n[7];
                    n[7] = n[8];
                    n[8] = get_cell(x+1, y+1, pgrid);
                }
            }

            /* how old is our cell? */
            age  = n[4];

            /* calculate the cell based on given neighbour information */
            cell = check_cell(n);

            /* is the actual cell alive? */
            if(cell){
                population++;
                /* prevent overflow */
                if(age<252)
                    age++;
                set_cell_age(x, y, age, pnext_grid);
            }
            else
                set_cell_age(x, y, 0, pnext_grid);
#if 0
            DEBUGF("x=%d,y=%d\n", x, y);
            DEBUGF("cell: %d\n", cell);
            DEBUGF("%d %d %d\n", n[0],n[1],n[2]);
            DEBUGF("%d %d %d\n", n[3],n[4],n[5]);
            DEBUGF("%d %d %d\n", n[6],n[7],n[8]);
            DEBUGF("----------------\n");
#endif
        }
    }
    generation++;
}



/**********************************/
/* this is the plugin entry point */
/**********************************/
enum plugin_status plugin_start(const void* parameter)
{
    int button = 0;
    int quit = 0;
    int stop = 0;
    int usb = 0;
    int pattern = 0;
    char *pgrid;
    char *pnext_grid;
    char *ptemp;
    (void)(parameter);

    backlight_force_on(); /* backlight control in lib/helper.c */
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(182, 198, 229)); /* rockbox blue */
#else
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif /* HAVE_LCD_COLOR */
#endif /* LCD_DEPTH > 1 */

    /* link pointers to grids */
    pgrid      = (char *)grid_a;
    pnext_grid = (char *)grid_b;

    init_grid(pgrid);

    if( parameter == NULL )
    {
        setup_grid(pgrid, pattern++);
    }
    else
    {
        if( load_cellfile(parameter, pgrid) )
        {
            rb->splashf( 1*HZ, "Cells loaded (%s)", (char *)parameter );
        }
        else
        {
            rb->splash( 1*HZ, "File Open Error");
            setup_grid(pgrid, pattern++);  /* fall back to stored patterns */
        }
    }


    show_grid(pgrid);

    while(!quit) {
        button = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts, 2);
        switch(button) {
        case ROCKLIFE_NEXT:
        case ROCKLIFE_NEXT_REP:
            /* calculate next generation */
            next_generation(pgrid, pnext_grid);
            /* swap buffers, grid is the new generation */
            ptemp = pgrid;
            pgrid = pnext_grid;
            pnext_grid = ptemp;
            /* show new generation */
            show_grid(pgrid);
            break;
        case ROCKLIFE_PLAY_PAUSE:
            stop = 0;
            while(!stop){
                /* calculate next generation */
                next_generation(pgrid, pnext_grid);
                /* swap buffers, grid is the new generation */
                ptemp = pgrid;
                pgrid = pnext_grid;
                pnext_grid = ptemp;
                /* show new generation */
                rb->yield();
                show_grid(pgrid);
                button = pluginlib_getaction(0, plugin_contexts, 2);
                switch(button) {
                case ROCKLIFE_PLAY_PAUSE:
                case ROCKLIFE_QUIT:
                    stop = 1;
                    break;
                default:
                    if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                        stop = 1;
                        quit = 1;
                        usb = 1;
                    }
                    break;
                }
                rb->yield();
            }
            break;
        case ROCKLIFE_INIT:
            init_grid(pgrid);
            setup_grid(pgrid, pattern);
            show_grid(pgrid);
            pattern++;
            pattern%=5;
            break;
        case ROCKLIFE_STATUS:
            status_line = !status_line;
            show_grid(pgrid);
            break;
        case ROCKLIFE_QUIT:
            /* quit plugin */
            quit = 1;
            break;
        default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                quit = 1;
                usb = 1;
            }
            break;
        }
        rb->yield();
    }

    backlight_use_settings(); /* backlight control in lib/helper.c */
    return usb? PLUGIN_USB_CONNECTED: PLUGIN_OK;
}
