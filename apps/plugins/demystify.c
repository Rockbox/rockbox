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
#include "pluginlib_actions.h"
#include "helper.h"
PLUGIN_HEADER

#define DEFAULT_WAIT_TIME 3
#define DEFAULT_NB_POLYGONS 7
#define NB_POINTS 4
#define MAX_STEP_RANGE 7
#define MIN_STEP_RANGE 3
#define MAX_POLYGONS 40
#define MIN_POLYGONS 1

/* Key assignement */
#define DEMYSTIFY_QUIT PLA_QUIT

#define DEMYSTIFY_INCREASE_SPEED PLA_RIGHT
#define DEMYSTIFY_DECREASE_SPEED PLA_LEFT
#define DEMYSTIFY_INCREASE_SPEED_REPEAT PLA_RIGHT_REPEAT
#define DEMYSTIFY_DECREASE_SPEED_REPEAT PLA_LEFT_REPEAT

#define DEMYSTIFY_ADD_POLYGON PLA_UP
#define DEMYSTIFY_REMOVE_POLYGON PLA_DOWN
#define DEMYSTIFY_ADD_POLYGON_REPEAT PLA_UP_REPEAT
#define DEMYSTIFY_REMOVE_POLYGON_REPEAT PLA_DOWN_REPEAT

const struct button_mapping *plugin_contexts[]
= {generic_directions, generic_actions,
#if defined(HAVE_REMOTE_LCD)
    remote_directions
#endif
};
#define NB_ACTION_CONTEXTS \
    sizeof(plugin_contexts)/sizeof(struct button_mapping*)
#ifdef HAVE_LCD_COLOR
struct line_color
{
    int r,g,b;
    int current_r,current_g,current_b;
};
#endif

/******************************* Globals ***********************************/

static const struct plugin_api* rb; /* global api struct pointer */

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

    backlight_use_settings(rb);
#ifdef HAVE_REMOTE_LCD
    remote_backlight_use_settings(rb);
#endif
}

#ifdef HAVE_LCD_COLOR
void color_randomize(struct line_color * color)
{
    color->r = rb->rand()%255;
    color->g = rb->rand()%255;
    color->b = rb->rand()%255;
}

void color_init(struct line_color * color)
{
    color_randomize(color);
    color->current_r=color->r;
    color->current_g=color->g;
    color->current_b=color->b;
}

void color_change(struct line_color * color)
{
    if(color->current_r<color->r)
        ++color->current_r;
    else if(color->current_r>color->r)
        --color->current_r;
    if(color->current_g<color->g)
        ++color->current_g;
    else if(color->current_g>color->g)
        --color->current_g;
    if(color->current_b<color->b)
        ++color->current_b;
    else if(color->current_b>color->b)
        --color->current_b;

    if(color->current_r==color->r &&
       color->current_g==color->g &&
       color->current_b==color->b)
        color_randomize(color);
}

#define COLOR_RGBPACK(color) \
    LCD_RGBPACK((color)->current_r, (color)->current_g, (color)->current_b)

void color_apply(struct line_color * color, struct screen * display)
{
    if (display->is_color){
        unsigned foreground=
            SCREEN_COLOR_TO_NATIVE(display,COLOR_RGBPACK(color));
        display->set_foreground(foreground);
    }
}
#endif

/*
 * Main function
 */

int plugin_main(void)
{
    int action;
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
        if (display->is_color)
            display->set_background(LCD_BLACK);
#endif
        fifo_init(&polygons[i]);
        polygon_move_init(&move[i]);
        polygon_init(&leading_polygon[i], rb->screens[i]);
    }

#ifdef HAVE_LCD_COLOR
    struct line_color color;
    color_init(&color);
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
            color_apply(&color, display);
#endif
            display->clear_display();
            polygons_draw(&polygons[i], display);
            display->update();
        }
#ifdef HAVE_LCD_COLOR
        color_change(&color);
#endif
        /* Speed handling*/
        if (sleep_time<0)/* full speed */
            rb->yield();
        else
            rb->sleep(sleep_time);
        action = pluginlib_getaction(rb, TIMEOUT_NOBLOCK,
                                     plugin_contexts, NB_ACTION_CONTEXTS);
        switch(action)
        {
            case DEMYSTIFY_QUIT:
                cleanup(NULL);
                return PLUGIN_OK;

            case DEMYSTIFY_ADD_POLYGON:
            case DEMYSTIFY_ADD_POLYGON_REPEAT:
                if(nb_wanted_polygons<MAX_POLYGONS)
                    ++nb_wanted_polygons;
                break;

            case DEMYSTIFY_REMOVE_POLYGON:
            case DEMYSTIFY_REMOVE_POLYGON_REPEAT:
                if(nb_wanted_polygons>MIN_POLYGONS)
                    --nb_wanted_polygons;
                break;

            case DEMYSTIFY_INCREASE_SPEED:
            case DEMYSTIFY_INCREASE_SPEED_REPEAT:
                if(sleep_time>=0)
                    --sleep_time;
                break;

            case DEMYSTIFY_DECREASE_SPEED:
            case DEMYSTIFY_DECREASE_SPEED_REPEAT:
                ++sleep_time;
                break;

            default:
                if (rb->default_event_handler_ex(action, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int ret;

    rb = api; /* copy to global api pointer */
    (void)parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    backlight_force_on(rb); /* backlight control in lib/helper.c */
#ifdef HAVE_REMOTE_LCD
    remote_backlight_force_on(rb); /* remote backlight control in lib/helper.c */
#endif
    ret = plugin_main();

    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
