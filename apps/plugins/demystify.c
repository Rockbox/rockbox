/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Kevin Ferrare
*
* Mystify demo plugin
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* Key assignement */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD)
#define DEMYSTIFY_QUIT BUTTON_MENU
#define DEMYSTIFY_ADD_POLYGON BUTTON_RIGHT
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_LEFT
#define DEMYSTIFY_INCREASE_SPEED BUTTON_SCROLL_FWD
#define DEMYSTIFY_DECREASE_SPEED BUTTON_SCROLL_BACK
#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define DEMYSTIFY_QUIT BUTTON_POWER
#define DEMYSTIFY_ADD_POLYGON BUTTON_RIGHT
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_LEFT
#define DEMYSTIFY_INCREASE_SPEED BUTTON_SCROLL_UP
#define DEMYSTIFY_DECREASE_SPEED BUTTON_SCROLL_DOWN
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define DEMYSTIFY_QUIT BUTTON_POWER
#define DEMYSTIFY_ADD_POLYGON BUTTON_RIGHT
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_LEFT
#define DEMYSTIFY_INCREASE_SPEED BUTTON_SCROLL_UP
#define DEMYSTIFY_DECREASE_SPEED BUTTON_SCROLL_DOWN
#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define DEMYSTIFY_QUIT BUTTON_POWER
#define DEMYSTIFY_ADD_POLYGON BUTTON_RIGHT
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_LEFT
#define DEMYSTIFY_INCREASE_SPEED BUTTON_UP
#define DEMYSTIFY_DECREASE_SPEED BUTTON_DOWN
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define DEMYSTIFY_QUIT BUTTON_A
#define DEMYSTIFY_ADD_POLYGON BUTTON_RIGHT
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_LEFT
#define DEMYSTIFY_INCREASE_SPEED BUTTON_UP
#define DEMYSTIFY_DECREASE_SPEED BUTTON_DOWN
#else
#define DEMYSTIFY_QUIT BUTTON_OFF
#define DEMYSTIFY_ADD_POLYGON BUTTON_UP
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_DOWN
#define DEMYSTIFY_INCREASE_SPEED BUTTON_RIGHT
#define DEMYSTIFY_DECREASE_SPEED BUTTON_LEFT
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define DEMYSTIFY_RC_QUIT BUTTON_RC_STOP
#define DEMYSTIFY_RC_ADD_POLYGON BUTTON_RC_BITRATE
#define DEMYSTIFY_RC_REMOVE_POLYGON BUTTON_RC_SOURCE
#define DEMYSTIFY_RC_INCREASE_SPEED BUTTON_RC_VOL_UP
#define DEMYSTIFY_RC_DECREASE_SPEED BUTTON_RC_VOL_DOWN
#endif
#endif

#define DEFAULT_WAIT_TIME 3
#define DEFAULT_NB_POLYGONS 7
#define NB_POINTS 4
#define MAX_STEP_RANGE 7
#define MIN_STEP_RANGE 3
#define MAX_POLYGONS 40
#define MIN_POLYGONS 1

#ifdef HAVE_LCD_COLOR
int r,g,b,rc,gc,bc;
#endif

/******************************* Globals ***********************************/

static struct plugin_api* rb; /* global api struct pointer */

/*
 * Compute a new random step to make the point bounce the borders of the screen
 */

int get_new_step(int step)
{
    if(step>0)
        return -(MIN_STEP_RANGE + rb->rand() % (MAX_STEP_RANGE-MIN_STEP_RANGE));
    else
        return (MIN_STEP_RANGE + rb->rand() % (MAX_STEP_RANGE-MIN_STEP_RANGE));
}

/*
 * Point Stuffs
 */

struct point
{
    int x;
    int y;
};

/*
 * Polygon Stuffs
 */

struct polygon
{
    struct point points[NB_POINTS];
};

/*
 * Generates a random polygon (which fits the screen size though)
 */
void polygon_init(struct polygon * polygon, struct screen * display)
{
    int i;
    for(i=0;i<NB_POINTS;++i)
    {
        polygon->points[i].x=(rb->rand() % (display->width));
        polygon->points[i].y=(rb->rand() % (display->height));
    }
}

/*
 * Draw the given polygon onto the screen
 */

void polygon_draw(struct polygon * polygon, struct screen * display)
{
    int i;
    for(i=0;i<NB_POINTS-1;++i)
    {
        display->drawline(polygon->points[i].x, polygon->points[i].y,
                         polygon->points[i+1].x, polygon->points[i+1].y);
    }
    display->drawline(polygon->points[0].x, polygon->points[0].y,
                     polygon->points[NB_POINTS-1].x,
                     polygon->points[NB_POINTS-1].y);
}

/*
 * Polygon moving data Stuffs
 */

struct polygon_move
{
    struct point move_steps[NB_POINTS];
};

void polygon_move_init(struct polygon_move * polygon_move)
{
    int i;
    for(i=0;i<NB_POINTS;++i)
    {
        polygon_move->move_steps[i].x=get_new_step(-1); 
        /* -1 because we want a positive random step */
        polygon_move->move_steps[i].y=get_new_step(-1);
    }
}

/*
 * Update the given polygon's position according to the given informations in
 * polygon_move (polygon_move may be updated)
 */
void polygon_update(struct polygon *polygon, struct screen * display, struct polygon_move *polygon_move)
{
    int i, x, y, step;
    for(i=0;i<NB_POINTS;++i)
    {
        x=polygon->points[i].x;
        step=polygon_move->move_steps[i].x;
        x+=step;
        if(x<=0)
        {
            x=1;
            polygon_move->move_steps[i].x=get_new_step(step);
        }
        else if(x>=display->width)
        {
            x=display->width-1;
            polygon_move->move_steps[i].x=get_new_step(step);
        }
        polygon->points[i].x=x;

        y=polygon->points[i].y;
        step=polygon_move->move_steps[i].y;
        y+=step;
        if(y<=0)
        {
            y=1;
            polygon_move->move_steps[i].y=get_new_step(step);
        }
        else if(y>=display->height)
        {
            y=display->height-1;
            polygon_move->move_steps[i].y=get_new_step(step);
        }
        polygon->points[i].y=y;
    }
}

/*
 * Polygon fifo Stuffs
 */

struct polygon_fifo
{
    int fifo_tail;
    int fifo_head;
    int nb_items;
    struct polygon tab[MAX_POLYGONS];
};

void fifo_init(struct polygon_fifo * fifo)
{
    fifo->fifo_tail=0;
    fifo->fifo_head=0;
    fifo->nb_items=0;
}

void fifo_push(struct polygon_fifo * fifo, struct polygon * polygon)
{
    if(fifo->nb_items>=MAX_POLYGONS)
        return;
    ++(fifo->nb_items);

    /*
     * Workaround for gcc (which uses memcpy internally) to avoid link error
     * fifo->tab[fifo->fifo_head]=polygon
     */
    rb->memcpy(&(fifo->tab[fifo->fifo_head]), polygon, sizeof(struct polygon));
    ++(fifo->fifo_head);
    if(fifo->fifo_head>=MAX_POLYGONS)
        fifo->fifo_head=0;
}

struct polygon * fifo_pop(struct polygon_fifo * fifo)
{
    int index;
    if(fifo->nb_items==0)
        return(NULL);
    --(fifo->nb_items);
    index=fifo->fifo_tail;
    ++(fifo->fifo_tail);
    if(fifo->fifo_tail>=MAX_POLYGONS)
        fifo->fifo_tail=0;
    return(&(fifo->tab[index]));
}

/*
 * Drawing stuffs
 */

void polygons_draw(struct polygon_fifo * polygons, struct screen * display)
{
    int i, j;
    for(i=0, j=polygons->fifo_tail;i<polygons->nb_items;++i, ++j)
    {
        if(j>=MAX_POLYGONS)
            j=0;
        polygon_draw(&(polygons->tab[j]), display);
    }
}

void cleanup(void *parameter)
{
    (void)parameter;

    rb->screens[SCREEN_MAIN]->backlight_set_timeout(rb->global_settings->backlight_timeout);
#if NB_SCREENS==2
    rb->screens[SCREEN_REMOTE]->backlight_set_timeout(rb->global_settings->remote_backlight_timeout);
#endif
}

#ifdef HAVE_LCD_COLOR
void new_color(void)
{
    r = rb->rand()%255;
    g = rb->rand()%255;
    b = rb->rand()%255;
}

void change_color(void)
{
    if(rc<r)
        ++rc;
    else if(rc>r)
        --rc;
    if(gc<g)
        ++gc;
    else if(gc>g)
        --gc;
    if(bc<b)
        ++bc;
    else if(bc>b)
        --bc;
    rb->lcd_set_foreground(LCD_RGBPACK(rc,gc,bc));
    if(rc==r && gc==g && bc==b)
        new_color();
}
#endif

/*
 * Main function
 */

int plugin_main(void)
{
    int button;
    int sleep_time=DEFAULT_WAIT_TIME;
    int nb_wanted_polygons=DEFAULT_NB_POLYGONS;
    int i;
    struct polygon_fifo polygons[NB_SCREENS];
    struct polygon_move move[NB_SCREENS]; /* This describes the movement of the leading
                                             polygon, the others just follow */
    struct polygon leading_polygon[NB_SCREENS];
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_LCD_COLOR
        struct screen *display = rb->screens[i];
        if (display->depth > 8)
            display->set_background(LCD_BLACK);
#endif
        fifo_init(&polygons[i]);
        polygon_move_init(&move[i]);
        polygon_init(&leading_polygon[i], rb->screens[i]);
    }

#ifdef HAVE_LCD_COLOR
    new_color();
    rc = r;
    gc = g;
    bc = b;
#endif

    while (true)
    {
        FOR_NB_SCREENS(i)
        {
            struct screen * display=rb->screens[i];
            if(polygons[i].nb_items>nb_wanted_polygons)
            {   /* We have too many polygons, we must drop some of them */
                fifo_pop(&polygons[i]);
            }
            if(nb_wanted_polygons==polygons[i].nb_items)
            {   /* We have the good number of polygons, we can safely drop 
                the last one to add the new one later */
                fifo_pop(&polygons[i]);
            }
            fifo_push(&polygons[i], &leading_polygon[i]);

            /*
            * Then we update the leading polygon for the next round acording to
            * current move (the move may be altered in case of sreen border 
            * collision)
            */
            polygon_update(&leading_polygon[i], display, &move[i]);

            /* Now the drawing part */

#ifdef HAVE_LCD_COLOR
            if (display->depth > 8)
                display->set_foreground(SCREEN_COLOR_TO_NATIVE(display,
                                        LCD_RGBPACK(rc, gc, bc)));
#endif
            display->clear_display();
            polygons_draw(&polygons[i], display);
            display->update();
        }
#ifdef HAVE_LCD_COLOR
        change_color();
#endif
        /* Speed handling*/
        if (sleep_time<0)/* full speed */
            rb->yield();
        else
            rb->sleep(sleep_time);

        /* Handle the user events */
        button = rb->button_get(false);
        switch(button)
        {
#ifdef DEMYSTIFY_RC_QUIT
            case DEMYSTIFY_RC_QUIT :
#endif
            case DEMYSTIFY_QUIT:
                cleanup(NULL);
                return PLUGIN_OK;
#ifdef DEMYSTIFY_RC_ADD_POLYGON
            case DEMYSTIFY_RC_ADD_POLYGON:
#endif
            case DEMYSTIFY_ADD_POLYGON:
                if(nb_wanted_polygons<MAX_POLYGONS)
                    ++nb_wanted_polygons;
                break;

#ifdef DEMYSTIFY_RC_REMOVE_POLYGON
            case DEMYSTIFY_RC_REMOVE_POLYGON:
#endif
            case DEMYSTIFY_REMOVE_POLYGON:
                if(nb_wanted_polygons>MIN_POLYGONS)
                    --nb_wanted_polygons;
                break;

#ifdef DEMYSTIFY_RC_INCREASE_SPEED
            case DEMYSTIFY_RC_INCREASE_SPEED:
#endif
            case DEMYSTIFY_INCREASE_SPEED:
                if(sleep_time>=0)
                    --sleep_time;
                break;

#ifdef DEMYSTIFY_RC_DECREASE_SPEED
            case DEMYSTIFY_RC_DECREASE_SPEED:
#endif
            case DEMYSTIFY_DECREASE_SPEED:
                ++sleep_time;
                break;

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int ret;

    rb = api; /* copy to global api pointer */
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    (void)parameter;
    if (rb->global_settings->backlight_timeout > 0)
    {
        int i;
        FOR_NB_SCREENS(i)
            rb->screens[i]->backlight_set_timeout(1);/* keep the light on */
    }
    ret = plugin_main();

    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
