/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, (c) 2018 Sebastian Leonhardt
 *
 * Original work (C) 2000 David Leonard, public domain according to
 * http://www.adaptive-enterprises.com.au/~d/software/amaze/
 *
 * Original Rockbox port by Jerry Chapman, 2007
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

#include "plugin.h"
#include "lib/pluginlib_actions.h"

const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* Option struct (stolen from clock_menu.c)  */
static const struct opt_items noyes_text[] = {
    { "No", -1 },
    { "Yes", -1 }
};

static const struct opt_items mazesize_text[] = {
    { "Easy", -1 },
    { "Medium", -1 },
    { "Hard", -1 },
    { "Expert", -1}
};

bool show_map;
bool remember_visited;
bool use_large_tiles;
int maze_size;

#if LCD_DEPTH >= 16
#define COLOR_GROUND    LCD_RGBPACK(51,51,51)
#define COLOR_SKY       LCD_RGBPACK(51,51,102)
#define COLOR_VISITED   LCD_RGBPACK(102,77,51)
#define COLOR_MARK      LCD_RGBPACK(255,100,61)
#define COLOR_GOAL      LCD_RGBPACK(128,0,0) /* red */
#define COLOR_COMPASS   LCD_RGBPACK(255,255,0) /* yellow */
#define COLOR_PARA      LCD_RGBPACK(153,153,102) /* color side wall */
/* #define COLOR_PERP   LCD_RGBPACK(102,51,0) */
#define COLOR_PERP      LCD_RGBPACK(204,153,102) /* color wall perp */
#elif LCD_DEPTH == 2
#define COLOR_GROUND    LCD_BLACK
#define COLOR_SKY       LCD_WHITE
#define COLOR_VISITED   LCD_DARKGRAY
#define COLOR_GOAL      LCD_WHITE
#define COLOR_COMPASS   LCD_BLACK
#define COLOR_MARK      LCD_WHITE
#define COLOR_PARA      LCD_DARKGRAY /* color side wall */
#define COLOR_PERP      LCD_LIGHTGRAY /* color wall perp */
#else /* mono */
#define COLOR_GROUND    LCD_BLACK
#define COLOR_SKY       LCD_BLACK
#define COLOR_VISITED   LCD_BLACK
#define COLOR_GOAL      LCD_BLACK
#define COLOR_COMPASS   LCD_WHITE
#define COLOR_PARA      LCD_BLACK /* color side wall */
#define COLOR_PERP      LCD_WHITE /* color wall perp */
#endif

#define A_DOWN  'v'
#define A_RIGHT '>'
#define A_UP    '^'
#define A_LEFT  '<'
#define SPACE   ' '    /* Space you can walk through */
#define BLOCK   'B'    /* A block that you can't walk through */
#define OBSPACE '#'    /* Obscured space */
#define VISITED '.'    /* Visited space */
#define GOAL    '%'    /* Exit from the maze */
#define START   '+'    /* Starting point in the maze */

enum dir { DIR_DOWN=0, DIR_RIGHT=1, DIR_UP=2, DIR_LEFT=3 };
#define _TOD(d) (enum dir)((d) % 4)
#define _TOI(d) (int)(d)
#define LEFT_OF(d) _TOD(_TOI(d) + 1)
#define REVERSE_OF(d) _TOD(_TOI(d) + 2)
#define RIGHT_OF(d) _TOD(_TOI(d) + 3)

static struct { int y, x; } dirtab[4] =
    { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
static char ptab[4] = { A_DOWN, A_RIGHT, A_UP, A_LEFT };

int px, py;             /* Player position */
enum dir pdir;          /* Player direction */
char punder;            /* character under player, if any */
int sx, sy;             /* Start position */
int gx, gy;             /* Goal position */
int gdist;              /* Distance from start to goal */
int won = 0;            /* Reached goal */
int cheated = 0;        /* Cheated somehow */
bool compass = false;   /* show compass */
int button;             /* Button data */
bool loaded=false;      /* Loaded? */

#define FIELD_SIZE 80
#define MAP_CONST 20

static char map[FIELD_SIZE * FIELD_SIZE]; /* map storage */
static char umap[FIELD_SIZE * FIELD_SIZE];

/* visible depth */
#define MAX_DEPTH  20
int depth;

/* CX,CY = center of screen */
#define CX LCD_WIDTH / 2
#define CY LCD_HEIGHT / 2

int crd_x[MAX_DEPTH], crd_y[MAX_DEPTH]; /* screen vertices */

#if LCD_WIDTH >= 220 || LCD_HEIGHT >= 220
#include "pluginbitmaps/amaze_tiles_9.h"
#define  amaze_tiles_large  amaze_tiles_9
#define  TILESIZE_LARGE     BMPWIDTH_amaze_tiles_9
#include "pluginbitmaps/amaze_tiles_7.h"
#define  amaze_tiles_small  amaze_tiles_7
#define  TILESIZE_SMALL     BMPWIDTH_amaze_tiles_7
#elif LCD_WIDTH >= 160 || LCD_HEIGHT >= 160
#include "pluginbitmaps/amaze_tiles_7.h"
#define  amaze_tiles_large  amaze_tiles_7
#define  TILESIZE_LARGE     BMPWIDTH_amaze_tiles_7
#include "pluginbitmaps/amaze_tiles_5.h"
#define  amaze_tiles_small  amaze_tiles_5
#define  TILESIZE_SMALL     BMPWIDTH_amaze_tiles_5
#else
#include "pluginbitmaps/amaze_tiles_5.h"
#define  amaze_tiles_large  amaze_tiles_5
#define  TILESIZE_LARGE     BMPWIDTH_amaze_tiles_5
#include "pluginbitmaps/amaze_tiles_3.h"
#define  amaze_tiles_small  amaze_tiles_3
#define  TILESIZE_SMALL     BMPWIDTH_amaze_tiles_3
#endif

/* save file names */
#define UMAP_FILE PLUGIN_GAMES_DIR "/amaze_umap.sav"
#define MAP_FILE  PLUGIN_GAMES_DIR "/amaze_map.sav"
#define PREF_FILE PLUGIN_GAMES_DIR "/amaze_prefs.sav"

void clearscreen (void)
{
#if LCD_DEPTH > 1
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();
}

void getmaxyx(int *y, int *x)
{
    *y = (maze_size + 1) * MAP_CONST;
    *x = (maze_size + 1) * MAP_CONST;
}

void map_write (char *pane, int y, int x, char c)
{
    int maxy, maxx;

    getmaxyx(&maxy, &maxx);

    if (x<0 || x>=maxx || y<0 || y>=maxy) return;

    pane[x * FIELD_SIZE + y] = c;

}
char map_read (char *pane, int y, int x)
{
    int maxy, maxx;

    getmaxyx(&maxy, &maxx);

    if (x<0 || x>=maxx || y<0 || y>=maxy) {
        return SPACE;
    }

    return pane[x * FIELD_SIZE + y];
}

/* redefine ncurses werase */
void werase (char *pane)
{
    int y, x;
    int maxy, maxx;

    getmaxyx(&maxy, &maxx);

    for (y = 0; y < maxy; y++)
        for (x = 0; x < maxx; x++)
            map_write(pane, y, x, SPACE);
}

/* start of David Leonard's code */

/* Look at position (y,x) in the maze map */
char at(int y, int x)
{
    int maxy, maxx;

    getmaxyx(&maxy, &maxx);

    if (y == py && x == px)
        return punder;

    if (y < 0 || y >= maxy || x < 0 || x >= maxx)
        return SPACE;
    else {
        return map_read(map, y, x);
    }
}

void copyumap(int y, int x, int fullvis)
{
    char c;

    c = at(y, x);
    if (!fullvis && c == SPACE && map_read(umap, y, x) != SPACE)
        c = OBSPACE;
    map_write(umap, y, x, c);
}

struct path {
    int y, x;
    int ttl;        /* Time until this path stops */
    int spawns;     /* Max number of forks this path can do */
    int distance;       /* Distance from start */
    struct path *next;
};

/*
 * A better maze-digging algorithm.
 * Simultaneously advance multiple digging paths through the map.
 * This is done by having a work queue of advancing paths.
 * Occasionally a path can fork; thus adding more to the work
 * queue and diversifying the maze.
 */
void eatmaze(int starty, int startx)
{
    struct path {
        int y, x;
        int ttl;        /* Time until this path stops */
        int spawns;     /* Max number of forks this path can do */
        int distance;       /* Distance from start */
        struct path *next;
    };
    struct path *path_free, *path_head, *path_tail;
    struct path *p, *s;
    /* static -- per <hcs> in #rockbox -- was having stack issues */
    static struct path path_storage[2000];
    int try;
    unsigned i;
    int y, x, dy, dx;
    int sdir;

    /* Set up the free list of path cells */
    for (i = 2; i < sizeof path_storage / sizeof path_storage[0]; i++)
        path_storage[i].next = &path_storage[i-1];
    path_storage[1].next = NULL;
    path_free = &path_storage[sizeof path_storage / sizeof path_storage[0] - 1];

    /* Set up the initial path cell */
    path_storage[0].y = starty;
    path_storage[0].x = startx;
    map_write(map, starty, startx, SPACE);

    /* Set up the initial goal. It will move later. */
    gy = starty;
    gx = startx;
    gdist = 0;

    /* Initial properties of the root path */
    path_storage[0].ttl = 50;
    path_storage[0].spawns = 20;
    path_storage[0].distance = 0;

    /* Put the initial path into the queue */
    path_storage[0].next = NULL;
    path_head = path_tail = &path_storage[0];

    while (path_head != NULL) {
        /* Dequeue */
        p = path_head;
        path_head = p->next;
        if (path_head == NULL)
            path_tail = NULL;

        /* There's a large chance that some paths miss a turn */
        if (rb->rand() % 100 < 60)
            goto requeue;

        /* First thing we do is advance the path. */
        y = p->y;
        x = p->x;

        sdir = rb->rand() % 4;
        for (try = 0; try < 4; try ++) {
            dx = dirtab[(sdir + try) % 4].x;
            dy = dirtab[(sdir + try) % 4].y;

            /* Going back on ourselves? */
            if (at(y + dy, x + dx) != BLOCK)
                continue;

            /* Connecting to another path? */
            if (at(y + dy * 2, x + dx * 2) != BLOCK)
                continue;
            if (at(y + dy + dx, x + dx - dy) != BLOCK)
                continue;
            if (at(y + dy - dx, x + dx + dy) != BLOCK)
                continue;

            break;
        }
        if (try == 4 || p->ttl <= 0) {
            /* Failed: the path is placed on the free list. */
            p->next = path_free;
            path_free = p;
            continue;
        }

        /* Dig the path a bit */
        p->y = y + dy;
        p->x = x + dx;
        map_write(map, p->y, p->x, SPACE);
        p->ttl--;
        p->distance++;

        if (p->distance > gdist) {
            gx = p->x;
            gy = p->y;
            gdist = p->distance;
        }

        /* Decide if we should spawn */
        if (/* rb->rand() % (p->ttl + 1) < p->spawns && */ path_free) {
            /* Take a new path element off the free list */
            s = path_free;
            path_free = s->next;

            /* Insert it at the tail of the queue */
            s->next = NULL;
            if (path_tail) path_tail->next = s;
            else           path_head = s;
            path_tail = s;

            /* Newly spawned path s will inherit most properties from p */
            s->y = p->y;
            s->x = p->x;
            s->ttl = p->ttl + rb->rand() % 10;
            s->spawns = p->spawns;
            s->distance = p->distance;

            /* p->spawns--; */
        }

    requeue:
        /* Put p onto the tail of the queue */
        p->next = NULL;
        if (path_tail) path_tail->next = p;
        else           path_head = p;
        path_tail = p;
    }
}

/* Move the player to a new position/direction in the maze map */
void mappmove(int newpy, int newpx, enum dir newpdir)
{
    map_write(map, py, px, punder);
    copyumap(py, px, 1);
    punder = at(newpy, newpx);
    py = newpy;
    px = newpx;
    pdir = newpdir;
    copyumap(py, px, 1);
    map_write(umap, py, px, ptab[_TOI(pdir)]);
    rb->lcd_update();
}

void clearmap (char *amap)
{
    int maxy, maxx;
    int y, x;

    getmaxyx(&maxy, &maxx);


    for (y = 0; y < maxy; y++)
        for (x = 0; x < maxx; x++)
            map_write(amap, y, x, BLOCK);
}

/* Reveal the solution to the user */
void showmap(void)
{
    int maxy, maxx, y, x;
    char ch, och;

    getmaxyx(&maxy, &maxx);
    for (y = 0; y < maxy; y++)
        for (x = 0; x < maxx; x++) {
            ch = at(y, x);
            if (ch == SPACE) {
                och = map_read(umap, y, x);
                if (och == BLOCK || och == OBSPACE)
                    ch = OBSPACE;
            }
            map_write(umap, y, x, ch);
            rb->yield();
        }
    map_write(umap, py, px, ptab[_TOI(pdir)]);
    rb->lcd_update();
}

/*
 * Create a new maze map
 * The algorithm here is quite inferior to that presented in the
 * magazine. I could only remember the gist of it: recursively dig a
 * trail that doesn't touch any other part of the maze, keeping track
 * of all possible points where the path could fork. Later on try those
 * possible branches; put limits on the segment lengths etc.
 */
void makemaze(void)
{
    int maxy, maxx;
    int i;

    /* Get the window dimensions */
    getmaxyx(&maxy, &maxx);

    clearmap(map);

    py = rb->rand() % (maxy - 2) + 1; /* maxy/2 */
    px = rb->rand() % (maxx - 2) + 1; /* maxx/2 */

    eatmaze(py, px);

    sx = px; /* starting position */
    sy = py;

    /* Face in an interesting direction: */
    pdir = DIR_UP;
    for (i = 0;
         i < 4 && at(py + dirtab[pdir].y,
                     px + dirtab[pdir].x) == BLOCK;
         i++)
        pdir = LEFT_OF(pdir);

    map_write(map, py, px, START);
    map_write(map, gy, gx, GOAL);
    punder = START;
    mappmove(py, px, pdir);
}

/* new drawing routines */

void draw_arrow(int dir, int sx, int sy, int pass)
{
    if (pass > 2) return;

    rb->lcd_fillrect(sx, sy, 1, 1);
    switch(dir) {
    case 0: /* down */
        rb->lcd_fillrect(sx + 2*pass, sy, 1, 1);
        draw_arrow(dir, sx - 1, sy - 1, pass + 1);
        break;
    case 2: /* up */
        rb->lcd_fillrect(sx + 2*pass, sy, 1, 1);
        draw_arrow(dir, sx - 1, sy + 1, pass + 1);
        break;
    case 1: /* left */
        rb->lcd_fillrect(sx, sy + 2*pass, 1, 1);
        draw_arrow(dir, sx + 1, sy - 1, pass + 1);
        break;
    case 3: /* right */
        rb->lcd_fillrect(sx, sy + 2*pass, 1, 1);
        draw_arrow(dir, sx - 1, sy - 1, pass +1);
        break;
    }
}

/* Provide a compass pointing 'north' or draw arrow mark on the floor */
void draw_pointer(int dir, bool is_compass)
{
    int offset;

    if(is_compass)
        offset = -crd_y[1]*2/3;   /* draw compass at the top */
    else
        offset = crd_y[1]/2;    /* draw mark on the floor */

#if LCD_DEPTH > 1
    if(is_compass)
        rb->lcd_set_foreground(COLOR_COMPASS);
    else
        rb->lcd_set_foreground(COLOR_MARK);
#else
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
#endif
    switch(dir) {
    case 0: /* point down */
        draw_arrow(dir, CX - 1, CY + offset + 6, 0);
        rb->lcd_fillrect(CX - 1, CY + offset + 1, 1, 3);
        break;
    case 2: /* point up */
        draw_arrow(dir, CX - 1, CY + offset, 0);
        rb->lcd_fillrect(CX - 1, CY + offset + 3, 1, 3);
        break;
    case 1: /* point left */
        draw_arrow(dir, CX - 6, CY + offset + 6, 0);
        rb->lcd_fillrect(CX - 3, CY + offset + 6, 3, 1);
        break;
    case 3: /* point right */
        draw_arrow(dir, CX + 1, CY + offset + 6, 0);
        rb->lcd_fillrect(CX - 4, CY + offset + 6, 3, 1);
        break;
    }
#if LCD_DEPTH == 1
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
    rb->lcd_update();
}

void draw_end_wall(int bx, int by)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(COLOR_PERP);
    rb->lcd_fillrect(CX - bx/2, CY - by/2, bx + 1, by);
#else
    rb->lcd_drawrect(CX - bx/2, CY - by/2, bx + 1, by);
#endif
}

void draw_side(int fx, int bx, int by, int tan_n, int tan_d, bool isleft)
{
    int signx;

    if(isleft)
        signx = -1;
    else
        signx = 1;

#if LCD_DEPTH > 1
    for(int i = bx; i < fx + 1; i++)
    {
        /* add some stripes */
        if(i % 3 == 0)
            rb->lcd_set_foreground(COLOR_PERP);
        else
            rb->lcd_set_foreground(COLOR_PARA);
        rb->lcd_vline(CX + signx * i/2,
                      CY - tan_n * (i-bx)/2 / tan_d - by/2,
                      CY + tan_n * (i-bx)/2 / tan_d + by/2);
    }
#else
    rb->lcd_vline(CX + signx * bx/2,
                  CY - by/2,
                  CY + by/2);
    rb->lcd_vline(CX + signx * (fx + 1)/2,
                  CY - tan_n * (fx - bx + 1)/2 / tan_d - by/2,
                  CY + tan_n * (fx - bx + 1)/2 / tan_d + by/2);
    rb->lcd_drawline(CX + signx * bx/2,
                     CY - by/2,
                     CX + signx * (fx + 1)/2,
                     CY - tan_n * (fx - bx + 1)/2 / tan_d - by/2);
    rb->lcd_drawline(CX + signx * bx/2,
                     CY + by/2,
                     CX + signx * (fx + 1)/2,
                     CY + tan_n * (fx - bx + 1)/2 / tan_d + by/2);
#endif
}

void draw_hall(int fx, int bx, int by, bool isleft)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(COLOR_PERP);

    if(isleft)
        rb->lcd_fillrect(CX - fx/2, CY - by/2, (fx - bx)/2 + 1, by);
    else
        rb->lcd_fillrect(CX + bx/2, CY - by/2, (fx - bx)/2 + 1, by);
#else
    if(isleft)
        rb->lcd_drawrect(CX - fx/2, CY - by/2, (fx - bx)/2 + 1, by);
    else
        rb->lcd_drawrect(CX + bx/2, CY - by/2, (fx - bx)/2 + 1, by);
#endif
}

void draw_side_tri(int fx, int fy, int bx, int tan_n, int tan_d,
                   bool isvisited, bool isgoal)
{
    int i;
    int signx, signy;

    signy = 1;

    while(signy >= -1)
    {
#if LCD_DEPTH > 1
        if(signy == 1)
            if(isgoal)
                rb->lcd_set_foreground(COLOR_GOAL);
            else if(isvisited)
                rb->lcd_set_foreground(COLOR_VISITED);
            else
                rb->lcd_set_foreground(COLOR_GROUND);
        else
            rb->lcd_set_foreground(COLOR_SKY);
#endif

        signx = 1;

        while(signx >= -1)
        {
            for(i = fx; i > bx; i--) {
#if LCD_DEPTH == 1 /* if unvisited floor draw pattern, otherwise solid */
                if (signy!=1 || isgoal || isvisited || (CX + signx * i/2) & 1)
#endif
                    rb->lcd_vline(CX + signx * i/2,
                              CY + signy * fy/2,
                              CY + signy * fy/2
                              + signy * tan_n
                              * (i - fx)/2 / tan_d);
            }
            signx-=2;
        }
        signy-=2;
    }
}

void draw_hall_crnr(int fx, int fy, int bx, int by,
                    bool isleft, bool isvisited, bool isgoal)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(COLOR_SKY);
#endif
    if(isleft)
        rb->lcd_fillrect(CX - fx/2, CY - fy/2,
                         (fx - bx)/2 + 1, (fy - by)/2);
    else
        rb->lcd_fillrect(CX + bx/2, CY - fy/2,
                         (fx - bx)/2 + 1, (fy - by)/2);

#if LCD_DEPTH > 1
    if(isgoal)
        rb->lcd_set_foreground(COLOR_GOAL);
    else if(isvisited)
        rb->lcd_set_foreground(COLOR_VISITED);
    else
        rb->lcd_set_foreground(COLOR_GROUND);

    if(isleft)
        rb->lcd_fillrect(CX - fx/2, CY + by/2,
                         (fx - bx)/2 + 1, (fy - by)/2);
    else
        rb->lcd_fillrect(CX + bx/2, CY + by/2,
                         (fx - bx)/2 + 1, (fy - by)/2);
#else /* LCD_DEPTH == 1 */
    /* if unvisited floor draw pattern, otherwise solid */
    if(isleft)
        for (int x = CX-fx/2; x <= CX-bx/2; x++) {
            if (isgoal || isvisited || x & 1)
                rb->lcd_vline(x, CY + by/2, CY + fy/2);
        }
    else
        for (int x = CX+bx/2; x <= CX+fx/2; x++) {
            if (isgoal || isvisited || x & 1)
                rb->lcd_vline(x, CY + by/2, CY + fy/2);
        }
#endif
}

void draw_center_sq(int fy, int bx, int by, bool isvisited, bool isgoal,
                    bool isfront, int chr)
{
    chr = chr - '0'; /* get the integer value */

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(COLOR_SKY);
#endif
    rb->lcd_fillrect(CX - bx/2, CY - fy/2, bx, (fy - by)/2);

#if LCD_DEPTH > 1
    if(isgoal)
        rb->lcd_set_foreground(COLOR_GOAL);
    else if(isvisited)
        rb->lcd_set_foreground(COLOR_VISITED);
    else
        rb->lcd_set_foreground(COLOR_GROUND);
    rb->lcd_fillrect(CX - bx/2, CY + by/2, bx, (fy - by)/2 + 1);
#else
     /* if unvisited floor draw pattern, otherwise solid */
    for (int x = CX-bx/2; x <= CX+bx/2; x++) {
        if (isgoal || isvisited || x & 1)
            rb->lcd_vline(x, CY + by/2, CY + fy/2 + 1);
    }
#endif

    if(isvisited && chr >= 0 && chr <= 3)
    {
#if LCD_DEPTH > 1
        rb->lcd_set_foreground(COLOR_MARK);
#else
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
#endif
        if(isfront)
        { /* cell is marked in front, draw arrow */
            if          (chr == ((int)(pdir) + 3) % 4)
                draw_pointer(DIR_LEFT, false);
            else if     (chr == ((int)(pdir) + 1) % 4)
                draw_pointer(DIR_RIGHT, false);
            else if     (chr == ((int)(pdir) + 2) % 4)
                draw_pointer(DIR_DOWN, false);
            else /* same direction */
                draw_pointer(DIR_UP, false);
        }
        else /* cell is marked but is in distance */
            rb->lcd_fillrect(CX, CY + (fy + by)/4, 2, 2);
#if LCD_DEPTH == 1
        rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
    }
}

bool is_visited(char cell)
{
    if (cell - '0' >= 0 && cell - '0' <= 3)
        return true;
    else if (cell == VISITED)
        return true;
    else
        return false;
}

void graphic_view(void)
{
    int dist;
    int x, y, dx, dy;
    int a, l, r; /* is block? ahead/left/right */
    bool g, gl, gr; /* ground visted? under/left/right */
    bool e, el, er; /* is goal? under/left/right */
    int tan_n, tan_d; /* tangent numerator/denominator */

    dx = dirtab[(int)pdir].x;
    dy = dirtab[(int)pdir].y;

    for (dist = 1; dist < depth; dist++)
        if (at(py + dy * dist, px + dx * dist) == BLOCK)
            break;

    if (!show_map)
    {
        clearmap(umap);
        copyumap(gy, gx, 1);
    }

#if LCD_DEPTH == 1
    clearscreen();
#endif

    while (--dist >= 0)
    {
        x = px + dx * dist;
        y = py + dy * dist;

        /* ground */
        g = is_visited(at(y, x));
        gl = is_visited(at(y - dx, x + dy));
        gr = is_visited(at(y + dx, x - dy));
        /* goal/end */
        e = at(y, x) == GOAL;
        el = at(y - dx, x + dy) == GOAL;
        er = at(y + dx, x - dy) == GOAL;
        /* ahead */
        a = at(y + dy, x + dx) == BLOCK;
        /* to the left */
        l = at(y - dx, x + dy) == BLOCK;
        /* to the right */
        r = at(y + dx, x - dy) == BLOCK;

        tan_n = crd_y[dist] - crd_y[dist+1];
        tan_d = crd_x[dist] - crd_x[dist+1];

        if (a)
            draw_end_wall(crd_x[dist+1], crd_y[dist+1]);
        if (l)
        {
            draw_side(crd_x[dist], crd_x[dist+1],
                      crd_y[dist+1], tan_n, tan_d, true);
        }
        else
        {
            draw_hall(crd_x[dist], crd_x[dist+1],
                      crd_y[dist+1], true);
            draw_hall_crnr(crd_x[dist], crd_y[dist], crd_x[dist+1],
                           crd_y[dist+1], true, gl, el);
        }
        if (r)
        {
            draw_side(crd_x[dist], crd_x[dist+1],
                      crd_y[dist+1], tan_n, tan_d, false);
        }
        else
        {
            draw_hall(crd_x[dist], crd_x[dist+1],
                      crd_y[dist+1], false);
            draw_hall_crnr(crd_x[dist], crd_y[dist],
                           crd_x[dist+1], crd_y[dist+1], false, gr, er);
        }

        draw_center_sq(crd_y[dist], crd_x[dist+1], crd_y[dist+1],
                       g, e, dist==0, (int)(at(y, x)));
        draw_side_tri(crd_x[dist], crd_y[dist],
                      crd_x[dist+1], tan_n, tan_d, g, e);

        copyumap(y + dy, x + dx, 0);    /* ahead */
        copyumap(y, x, 1);              /* here */
        copyumap(y - dx, x + dy, 0);    /* left */
        copyumap(y + dx, x - dy, 0);    /* right */
        if (!l)
            copyumap(y - dx + dy, x + dy + dx, 0); /* lft ahead */
        if (!r)
            copyumap(y + dx + dy, x - dy + dx, 0); /* rt ahead */
    }
    if (compass)
        draw_pointer(pdir, true);
}

void win(void)
{
    /*
      int i;
      char amazed[8] = "amazing!";
      char newton;

      for (i=0; i <= 8; i++)
      {
      newton = amazed[i];
      map_write(msg, 0, i + 31, newton);
      }
    */
    won++;
    showmap();
    show_map = 1;
}


/* Try to move the player in the direction given */
void trymove(enum dir dir)
{
    int nx, ny;

    ny = py + dirtab[(int)dir].y;
    nx = px + dirtab[(int)dir].x;

    if (at(ny, nx) == BLOCK)
    {
        graphic_view();
        return;
    }

    if (at(ny, nx) == GOAL)
        win();

    mappmove(ny, nx, pdir);
    if (remember_visited && punder == SPACE)
        punder = VISITED;
    graphic_view();
}

void walkleft(void)
{
    int a, l;
    int dx, dy;
    int owon = won;

    while (1)
    {
        int input = pluginlib_getaction(TIMEOUT_NOBLOCK, plugin_contexts,
                                        ARRAYLEN(plugin_contexts));
        if(input==PLA_CANCEL || input==PLA_EXIT)
        {
            return;
        }
        rb->lcd_update();
        if (won != owon)
        {
            break;
        }

        dx = dirtab[(int)pdir].x;
        dy = dirtab[(int)pdir].y;

        /* ahead */
        a = at(py + dy, px + dx) == BLOCK;
        /* to the left */
        l = at(py - dx, px + dy) == BLOCK;

        if (!l)
        {
            mappmove(py, px, LEFT_OF(pdir));
            graphic_view();
            rb->sleep(2);
            trymove(pdir);
            continue;
        }
        if (a)
        {
            mappmove(py, px, RIGHT_OF(pdir));
            graphic_view();
            continue;
        }
        trymove(pdir);
        rb->yield();
    }
}

void draw_tile(int index, int x, int y)
{
    if (use_large_tiles == 1)
        rb->lcd_bitmap_part (amaze_tiles_large, 0, index * TILESIZE_LARGE,
                             TILESIZE_LARGE, x * TILESIZE_LARGE, y * TILESIZE_LARGE,
                             TILESIZE_LARGE, TILESIZE_LARGE);
    else
        rb->lcd_bitmap_part (amaze_tiles_small, 0, index * TILESIZE_SMALL,
                             TILESIZE_SMALL, x * TILESIZE_SMALL, y * TILESIZE_SMALL,
                             TILESIZE_SMALL, TILESIZE_SMALL);
}

void draw_tile_map(int xmin, int xmax, int ymin, int ymax)
{
    int x,y;
    char map_unit;
    int tdex = 7; /* tile index */

    enum tile_index
    { t_down=0, t_right=1, t_up=2, t_left=3, t_visited=4,
      t_obspace=5, t_goal=6, t_block=7, t_space=8, t_start=9 };

    for(y = ymin; y <= ymax; y++)
        for(x = xmin; x <= xmax; x++)
        {

            map_unit = map_read(umap, y, x);

            switch (map_unit)
            {
            case VISITED:
            case '0': case '1': case '2': case '3':
                tdex = t_visited;
                break;
            case OBSPACE:
                tdex = t_obspace;
                break;
            case START:
                tdex = t_start;
                break;
            case GOAL:
                tdex = t_goal;
                break;
            case SPACE:
                tdex = t_space;
                break;
            case BLOCK:
                tdex = t_block;
                break;
            }
            draw_tile(tdex, x - xmin, y - ymin);
        }
    if(sx>=xmin && sx<=xmax && sy>=ymin && sy<=ymax)
    {
        x = sx;
        y = sy;
        draw_tile(t_start, x - xmin, y - ymin);
    }

    if(px>=xmin && px<=xmax && py>=ymin && py<=ymax)
    {
        x = px;
        y = py;
        draw_tile(pdir, x - xmin, y - ymin);
    }

    rb->lcd_update();
}

void check_map_bounds(int *xmin, int *xmax, int *ymin, int *ymax)
{
    int maxx, maxy;

    getmaxyx(&maxy, &maxx);

    /* bounds check x */
    if(*xmin < 0)
    {
        *xmax = *xmax - *xmin;
        *xmin = 0;
    }
    if(*xmax >= maxx)
    {
        *xmin = *xmin - *xmax + maxx - 1;
        *xmax = maxx - 1;
    }

    /* bounds check y */
    if(*ymin < 0)
    {
        *ymax = *ymax - *ymin;
        *ymin = 0;
    }
    if(*ymax >= maxy)
    {
        *ymin = *ymin - *ymax + maxy - 1;
        *ymax = maxy - 1;
    }
}

void calc_map_size(int *xmin, int *xmax, int *ymin, int *ymax)
{
    int tile_size; /* runtime option */
    int vx, vy; /* maxx, maxy of view */
    int midx, midy; /* midpoint x, y of view */
    int maxx, maxy; /* maxx, maxy of map */

    getmaxyx(&maxy, &maxx);

    if (use_large_tiles == 1)
        tile_size = TILESIZE_LARGE;
    else
        tile_size = TILESIZE_SMALL;

    vx = LCD_WIDTH / tile_size;
    if (vx > maxx)
        vx = maxx;
    vy = LCD_HEIGHT / tile_size;
    if (vy > maxy)
        vy = maxy;

    midx = vx / 2;
    midy = vy / 2;

    *xmin = px - midx;
    if(vx % 2 == 0)
        *xmax = px + midx - 1;
    else
        *xmax = px + midx;

    *ymin = py - midy;
    if(vy % 2 == 0)
        *ymax = py + midy - 1;
    else
        *ymax = py + midy;

}


void draw_portion_map(void)
{
    int xmin, xmax, ymin, ymax; /* coords of map corners */
    bool quit_map;
    int input;

    clearscreen();

    calc_map_size(&xmin, &xmax, &ymin, &ymax);

    quit_map = false;

    while (!quit_map)
    {
        check_map_bounds(&xmin, &xmax, &ymin, &ymax);
        draw_tile_map(xmin, xmax, ymin, ymax);

        input = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                                    ARRAYLEN(plugin_contexts));

        switch(input)
        {
        case PLA_CANCEL:
        case PLA_EXIT:
            quit_map = true;
            break;
        case PLA_UP:
        case PLA_UP_REPEAT:
            ymin--;
            ymax--;
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            ymin++;
            ymax++;
            break;
        case PLA_LEFT:
        case PLA_LEFT_REPEAT:
            xmin--;
            xmax--;
            break;
        case PLA_RIGHT:
        case PLA_RIGHT_REPEAT:
            xmin++;
            xmax++;
            break;
        default:
            break;
        }
    }
}

bool load_map(char *filename, char *amap)
{
    int fd;
    int x,y;
    int maxxy;
    size_t n;
    char newton = BLOCK;
    char map_size[2];

    /* load a map */
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        LOGF("Invalid map file: %s\n", filename);
        return false;
    }

    n = rb->read(fd, map_size, sizeof(map_size));
    if (n <= 0)
    {
        rb->splash(HZ*2, "Invalid map size.");
        return false;
    }

    maze_size = (int)map_size[0] - 48;
    maxxy = MAP_CONST * (maze_size + 1);
    char line[maxxy + 1];

    for(y=0; y < maxxy ; y++)
    {
        n = rb->read(fd, line, sizeof(line));
        if (n <= 0)
        {
            return false;
        }
        for(x=0; x < maxxy+1; x++)
        {
            switch(line[x])
            {
            case '\n':
                break;
            case '0': case '1': case '2': case '3':
                newton = line[x];
                break;
            case START:
                sy = y;
                sx = x;
                newton = START;
                break;
            case SPACE:
            case BLOCK:
            case OBSPACE:
                newton = line[x];
                break;
            case GOAL:
                newton = GOAL;
                gy = y;
                gx = x;
                break;
            case A_DOWN: case A_LEFT: case A_UP: case A_RIGHT:
                py = y;
                px = x;
                switch(line[x])
                {
                case A_DOWN:
                    pdir = DIR_DOWN;
                    break;
                case A_LEFT:
                    pdir = DIR_LEFT;
                    break;
                case A_UP:
                    pdir = DIR_UP;
                    break;
                case A_RIGHT:
                    pdir = DIR_RIGHT;
                    break;
                }
                /* FALLTHROUGH */
            case VISITED:
                newton = VISITED;
                break;
            }
            if (line[x] != '\n')
                map_write(amap, y, x, newton);
        }
    }
    rb->close(fd);
    rb->remove(filename);
    return true;
}

bool load_game(void)
{
    if (load_map(UMAP_FILE, umap) && load_map(MAP_FILE, map))
        return true;
    else
        return false;
}


bool save_map(char *filename, char *amap)
{
    int x,y;
    int maxy, maxx;
    char map_unit;
    int fd;
    int line_len = (maze_size + 1) * MAP_CONST + 1;
    char line[line_len];
    char map_size[2] =
        {'0','\n'};

    line[line_len - 1] = '\n'; /* last cell is a linefeed */
    map_size[0] = (char)(maze_size + 48);

    if ((fd = rb->open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
        return false;

    rb->write(fd, map_size, 2);

    getmaxyx(&maxy, &maxx);

    for(y=0; y < maxy; y++)
    {
        for (x=0; x < maxx; x++)
        {
            map_unit = map_read(amap, y, x);

            if(y == py && x == px)
                line[x] = ptab[_TOI(pdir)];
            else if(y == sy && x == sx)
                line[x] = START;
            else
                line[x] = map_unit;
        }
        rb->write(fd, line, line_len);
    }
    rb->close(fd);

    return true;
}

bool save_game(void)
{
    rb->splash(0, "Saving...");
    if (save_map(UMAP_FILE, umap) && save_map(MAP_FILE, map))
        return true;
    else
        return false;
}


bool ingame;
bool save_prefs(char *filename);

static int menu_cb(int action, const struct menu_item_ex *this_item)
{
    int idx=((intptr_t)this_item);
    if (action==ACTION_REQUEST_MENUITEM) {
        if (ingame) {
            if (idx==2)
                return ACTION_EXIT_MENUITEM;
        }
        else { /* !ingame */
            if (idx==3 || idx==8)
                return ACTION_EXIT_MENUITEM;
            if (!loaded && (idx==0 || idx==9))
                return ACTION_EXIT_MENUITEM;
        }
    }
    return action;
}

int menu(void)
{
    bool exit_menu = false;
    int selection = 0, result = 0, status = 1;

    MENUITEM_STRINGLIST(menu,"Amaze Menu", menu_cb, "Resume Game", "Start New Game",
                        "Set Maze Size",
                        "View Map", "Show Compass",
                        "Show Map", "Remember Path", "Use Large Tiles",
                        "Show Solution",
                        "Quit without Saving", "Quit");

    clearscreen();

    while(!exit_menu)
    {
        result = rb->do_menu(&menu, &selection, NULL, false);
        switch(result)
        {
        case 0:     /* resume */
            exit_menu = true;
            if (!ingame) {
                if (!save_prefs(PREF_FILE))
                    rb->splash(HZ, "Could not save preferences.");
            }
            break;
        case 1:     /* new game */
            exit_menu = true;
            if (ingame)
            {
                rb->splash(0, "Generating maze...");
                clearmap(umap);
                makemaze();

                /* Show where the goal is */
                copyumap(gy, gx, 1);
                rb->lcd_update();

                mappmove(py, px, pdir);

                if (remember_visited)
                    punder = VISITED;
                else
                    punder = SPACE;
            }
            else { /* !ingame */
                loaded=false;
                if (!save_prefs(PREF_FILE))
                    rb->splash(HZ, "Could not save preferences.");
            }
            break;
        case 2:     /* Set maze size */
            {
            int old_size = maze_size;
            rb->set_option("Set Maze Size", &maze_size, INT,
                           mazesize_text, 4, NULL);
            if (maze_size != old_size)
                loaded = false;
            }
            break;
        case 3:     /* View map */
            exit_menu = true;
            draw_portion_map();
            break;
        case 4:     /* Show compass option */
            rb->set_option("Show Compass", &compass, BOOL,
                           noyes_text, 2, NULL);
            break;
        case 5:     /* Show Map option */
            rb->set_option("Show Map", &show_map, BOOL,
                           noyes_text, 2, NULL);
            break;
        case 6:     /* Remember Path option */
            rb->set_option("Remember Path", &remember_visited, BOOL,
                           noyes_text, 2, NULL);
            break;
        case 7:     /* Tilesize option */
            rb->set_option("Use Large Tiles", &use_large_tiles, BOOL,
                           noyes_text, 2, NULL);
            break;
        case 8:     /* solver */
             exit_menu = true;
             cheated++;
             walkleft();
             break;
        case 9:     /* quit w/o saving */
            exit_menu = true;
            status = 0;
            break;
        case 10:     /* save+quit */
            exit_menu = true;
            if (ingame) {
                if (save_game())
                    status = 0;
                else
                    rb->splash(HZ*3, "Save Error");
            }
            else {
                if(loaded)
                    save_game();
                status = 0;
            }
            break;
        }
    }
    return status;
}

int amaze(void)
{
    int quitting;
    int i;
    int input;

    clearscreen();
    rb->lcd_setfont(FONT_SYSFIXED);
    if(!loaded)
        rb->splash(0, "Generating maze...");

    crd_x[0] = LCD_WIDTH + 1;
    crd_y[0] = LCD_HEIGHT + 1;
    for (depth=1; depth < MAX_DEPTH + 1; depth++)
    {
        crd_x[depth] = crd_x[depth-1]*2/3;
        if(crd_x[depth] % 2 != 0) crd_x[depth]++;
        crd_y[depth] = crd_y[depth-1]*2/3;
        if(crd_y[depth] % 2 != 0) crd_y[depth]++;
        if (crd_x[depth]==crd_x[depth-1] || crd_y[depth]==crd_y[depth-1])
            break;
    }
    --depth;

    if (!loaded)
    {
        clearmap(umap);
        makemaze();
    }

    /* Show where the goal is */

    copyumap(gy, gx, 1);

    rb->lcd_update();

    quitting = 0;

    mappmove(py, px, pdir);

    if (remember_visited)
        punder = VISITED;
    else
        punder = SPACE;

    clearscreen();
    graphic_view();

    while (!quitting && !won)
    {

        rb->lcd_update();

        input = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                                    ARRAYLEN(plugin_contexts));

        switch (input)
        {
        case PLA_CANCEL:
        case PLA_EXIT:
            i = menu();
            rb->lcd_setfont(FONT_SYSFIXED);
            clearscreen();
            graphic_view();

            switch (i)
            {
            case 0:
                quitting = 1;
                break;
            case 1:
                break;
            case 2:
                return 0;
                break;
            }
            break;
        case PLA_UP:
        case PLA_UP_REPEAT:
            trymove(pdir);
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            trymove(REVERSE_OF(pdir));
            break;
        case PLA_LEFT:
            mappmove(py, px, LEFT_OF(pdir));
            graphic_view();
            break;
        case PLA_RIGHT:
            mappmove(py, px, RIGHT_OF(pdir));
            graphic_view();
            break;
        case PLA_SELECT:
            if (punder==SPACE || punder==VISITED)
            {   /* mark ground */
                punder = pdir + '0';
            }
            else
            {   /* clear mark */
                if (remember_visited)
                    punder = VISITED;
                else
                    punder = SPACE;
            }
            graphic_view();
            break;
        }
    }

    rb->lcd_update();
    //graphic_view();
    if (won)
    {
        won = false; /* reset boolean */
        if (cheated)
        {
            rb->splash(HZ*2, "You cheated.");
            return 0;
        }
        rb->splash(HZ*3, "You win!");
        return 1;
    }
    else
    {
        return 0;
    }
}

bool save_prefs(char *filename)
{
    int fd;
    char ms[2] = { (char)(maze_size) + '0', '\n' };
    char sm[2] = { (char)(show_map) + '0', '\n' };
    char rv[2] = { (char)(remember_visited) + '0', '\n' };
    char lt[2] = { (char)(use_large_tiles) + '0', '\n' };

    fd = rb->open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if(fd >= 0)
    {
        rb->write(fd, ms, 2);
        rb->write(fd, sm, 2);
        rb->write(fd, rv, 2);
        rb->write(fd, lt, 2);
    }
    else
    {
        rb->splash(HZ, "Could not save preferences.");
        return false;
    }
    rb->close(fd);
    return true;
}

bool load_prefs(char *filename)
{
    int fd;
    char instr[2];

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        LOGF("Invalid preferences file: %s\n", filename);
        return false;
    }

    rb->read(fd, instr, sizeof(instr));
    maze_size = (int)(instr[0] - '0');
    rb->read(fd, instr, sizeof(instr));
    show_map = (bool)(instr[0] - '0');
    rb->read(fd, instr, sizeof(instr));
    remember_visited = (bool)(instr[0] - '0');
    rb->read(fd, instr, sizeof(instr));
    use_large_tiles = (bool)(instr[0] - '0');
    rb->close(fd);

    return true;
}

enum plugin_status plugin_start(const void *parameter)
{
    (void) parameter;
    int ret;

    rb->srand(*rb->current_tick);

    /* hard-code in program default options */
    show_map=1;
    remember_visited=1;
    use_large_tiles=1;
    maze_size=1;

    loaded=load_game();

    /* let's go, gentlemen, we have some work to do */
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    ingame = false;
    ret = menu();
    if (ret) {
        ingame = true;
        amaze();
    }

    rb->lcd_setfont(FONT_UI);

    return PLUGIN_OK;
}
