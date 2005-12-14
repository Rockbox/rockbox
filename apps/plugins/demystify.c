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

/* Key assignement */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_NANO_PAD)
#define DEMYSTIFY_QUIT BUTTON_MENU
#define DEMYSTIFY_ADD_POLYGON BUTTON_RIGHT
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_LEFT
#define DEMYSTIFY_INCREASE_SPEED BUTTON_SCROLL_FWD
#define DEMYSTIFY_DECREASE_SPEED BUTTON_SCROLL_BACK
#else
#define DEMYSTIFY_QUIT BUTTON_OFF
#define DEMYSTIFY_ADD_POLYGON BUTTON_UP
#define DEMYSTIFY_REMOVE_POLYGON BUTTON_DOWN
#define DEMYSTIFY_INCREASE_SPEED BUTTON_RIGHT
#define DEMYSTIFY_DECREASE_SPEED BUTTON_LEFT
#endif

#define DEFAULT_WAIT_TIME 3
#define DEFAULT_NB_POLYGONS 7
#define NB_POINTS 4
#define MAX_STEP_RANGE 7
#define MIN_STEP_RANGE 3
#define MAX_POLYGONS 40
#define MIN_POLYGONS 1

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
void polygon_init(struct polygon * polygon)
{
    int i;
    for(i=0;i<NB_POINTS;++i)
    {
        polygon->points[i].x=(rb->rand() % (LCD_WIDTH));
        polygon->points[i].y=(rb->rand() % (LCD_HEIGHT));
    }
}

/*
 * Draw the given polygon onto the screen
 */

void polygon_draw(struct polygon * polygon)
{
    int i;
    for(i=0;i<NB_POINTS-1;++i)
    {
        rb->lcd_drawline(polygon->points[i].x, polygon->points[i].y,
                         polygon->points[i+1].x, polygon->points[i+1].y);
    }
    rb->lcd_drawline(polygon->points[0].x, polygon->points[0].y,
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
void polygon_update(struct polygon *polygon, struct polygon_move *polygon_move)
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
        else if(x>=LCD_WIDTH)
        {
            x=LCD_WIDTH-1;
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
        else if(y>=LCD_HEIGHT)
        {
            y=LCD_HEIGHT-1;
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

void polygons_draw(struct polygon_fifo * polygons)
{
    int i, j;
    for(i=0, j=polygons->fifo_tail;i<polygons->nb_items;++i, ++j)
    {
        if(j>=MAX_POLYGONS)
            j=0;
        polygon_draw(&(polygons->tab[j]));
    }
}



static struct polygon_fifo polygons;
static struct polygon_move move; /* This describes the movement of the leading
                                    polygon, the others just follow */
static struct polygon leading_polygon;


void cleanup(void *parameter)
{
    (void)parameter;
    
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/*
 * Main function
 */

int plugin_main(void)
{
    int button;
    int sleep_time=DEFAULT_WAIT_TIME;
    int nb_wanted_polygons=DEFAULT_NB_POLYGONS;

    fifo_init(&polygons);
    polygon_move_init(&move);
    polygon_init(&leading_polygon);

    while (true)
    {
        if(polygons.nb_items>nb_wanted_polygons)
        {   /* We have too many polygons, we must drop some of them */
            fifo_pop(&polygons);
        }
        if(nb_wanted_polygons==polygons.nb_items)
        {   /* We have the good number of polygons, we can safely drop the last
               one to add the new one later */
            fifo_pop(&polygons);
        }
        fifo_push(&polygons, &leading_polygon);

        /*
         * Then we update the leading polygon for the next round acording to
         * current move (the move may be altered in case of sreen border 
         * collision)
         */
        polygon_update(&leading_polygon, &move);

        /* Now the drawing part */

        rb->lcd_clear_display();
        polygons_draw(&polygons);
        rb->lcd_update();

        /* Speed handling*/
        if (sleep_time<0)/* full speed */
            rb->yield();
        else
            rb->sleep(sleep_time);

        /* Handle the user events */
        button = rb->button_get(false);
        switch(button)
        {
            case (DEMYSTIFY_QUIT):
                cleanup(NULL);
                return PLUGIN_OK;

            case (DEMYSTIFY_ADD_POLYGON):
                if(nb_wanted_polygons<MAX_POLYGONS)
                    ++nb_wanted_polygons;
                break;

            case (DEMYSTIFY_REMOVE_POLYGON):
                if(nb_wanted_polygons>MIN_POLYGONS)
                    --nb_wanted_polygons;
                break;

            case (DEMYSTIFY_INCREASE_SPEED):
                if(sleep_time>=0)
                    --sleep_time;
                break;

            case (DEMYSTIFY_DECREASE_SPEED):
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
    /*
    * this macro should be called as the first thing you do in the plugin.
    * it test that the api version and model the plugin was compiled for
    * matches the machine it is running on
    */

    TEST_PLUGIN_API(api);

    rb = api; /* copy to global api pointer */
    (void)parameter;
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1);/* keep the light on */
        
    ret = plugin_main();

    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
