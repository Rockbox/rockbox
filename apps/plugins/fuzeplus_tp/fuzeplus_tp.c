#include "plugin.h"
#include "math.h"

#define REL_MULT 2

#define SEGMENT_INVALID -10
#define SEGMENT_TOPRIGHT 1
#define SEGMENT_BOTTOMRIGHT 2
#define SEGMENT_BOTTOMLEFT 3
#define SEGMENT_TOPLEFT 4

#define MAX_ITEMS 10

#define MIDDLEADJUST_FACTOR 30

volatile fuzeplus_touchpad_data * tp_data_ptr;

volatile int last_segment;
volatile int current_item;

volatile int middle_x, middle_y;

void fptp_calcscreencoords(int x, int y, int maxx, int maxy, int box_left, int box_right,
        int box_top, int box_bottom, int* resx, int* resy)
{
    *resx = (x * (box_right-box_left)) / maxx;
    *resy = (y * (box_bottom-box_top)) / maxy;
    *resy = (box_bottom-box_top) - *resy;
    *resx += box_left;
    *resy += box_top;

    return;
}

void fptp_drawdebugscreen(void)
{
    char buf[30];

    int box_left = 10;
    int box_right = (LCD_WIDTH - 10);
    int box_width = (box_right - box_left);
    int box_top = (LCD_HEIGHT/2);
    int box_height = ((box_width * tp_data_ptr->max_y) / tp_data_ptr->max_x);
    int box_bottom = box_top + box_height;

    //calculate pixel position
    int pixelx, pixely;
    fptp_calcscreencoords(tp_data_ptr->abs_x,
            tp_data_ptr->abs_y,
            tp_data_ptr->max_x,
            tp_data_ptr->max_y,
            box_left, box_right, box_top, box_bottom,
            &pixelx, &pixely);

    //calculate avg line position
    int avgx, avgy;
    fptp_calcscreencoords(middle_x, middle_y,
            tp_data_ptr->max_x,
            tp_data_ptr->max_y,
            box_left, box_right, box_top, box_bottom,
            &avgx, &avgy);

    //START DRAW

    //START DRAW DEBUG BOX
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
    rb->lcd_drawrect(box_left, box_top, box_width, box_height);
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
    rb->lcd_drawline(box_left, avgy, box_right, avgy);
    rb->lcd_drawline(avgx, box_top, avgx, box_bottom);

    if(tp_data_ptr->num_fingers)
    {
        //draw relative
        rb->lcd_set_foreground(LCD_RGBPACK(0,255,0));
        rb->lcd_drawline(pixelx, pixely, pixelx + (tp_data_ptr->rel_x*REL_MULT), pixely - (tp_data_ptr->rel_y*REL_MULT));

        //draw absolute
        rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));
        rb->lcd_fillrect(pixelx - 1, pixely - 1, 3, 3);
    }


    //START DRAW TEXT
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));

    rb->snprintf(buf, sizeof(buf), "abs_x: %d",
            tp_data_ptr->abs_x);
    rb->lcd_putsxy(0,0,buf);

    rb->snprintf(buf, sizeof(buf), "abs_y: %d",
            tp_data_ptr->abs_y);
    rb->lcd_putsxy(0,15,buf);

    rb->snprintf(buf, sizeof(buf), "rel_x: %d",
            tp_data_ptr->rel_x);
    rb->lcd_putsxy(0,30,buf);

    rb->snprintf(buf, sizeof(buf), "rel_y: %d",
            tp_data_ptr->rel_y);
    rb->lcd_putsxy(0,45,buf);

    rb->snprintf(buf, sizeof(buf), "max_x: %d",
            tp_data_ptr->max_x);
    rb->lcd_putsxy(0,60,buf);

    rb->snprintf(buf, sizeof(buf), "max_y: %d",
            tp_data_ptr->max_y);
    rb->lcd_putsxy(0,75,buf);

    rb->snprintf(buf, sizeof(buf), "num_fingers: %d",
            tp_data_ptr->num_fingers);
    rb->lcd_putsxy(0,90,buf);

    return;
}

void fptp_drawcircle(int x, int y, int radius)
{
    int angle = 0;
    long sin, cos;
    int incr = 1;

    for(angle=0; angle<360; angle+=incr)
    {
        sin = fp14_sin(angle)*radius;
        cos = fp14_cos(angle)*radius;

        int drx = x + (int)(sin>>14);
        int dry = y + (int)(cos>>14);

        rb->lcd_drawpixel(drx, dry);
    }

    return;
}

void fptp_drawvirtualwheel(int x, int y, int radius, int sourcex, int sourcey)
{
    int current_segment;
    if(tp_data_ptr->num_fingers != 1)
    {
        current_segment = SEGMENT_INVALID;
        middle_x = tp_data_ptr->max_x/2;
        middle_y = tp_data_ptr->max_y/2;
    }
    else
    {
        middle_x += sourcex/MIDDLEADJUST_FACTOR;
        middle_y += sourcey/MIDDLEADJUST_FACTOR;

        if(sourcex >= 0)
        {
            if(sourcey>=0) current_segment = SEGMENT_TOPRIGHT;
            else current_segment = SEGMENT_BOTTOMRIGHT;
        }
        else
        {
            if(sourcey>=0) current_segment = SEGMENT_TOPLEFT;
            else current_segment = SEGMENT_BOTTOMLEFT;
        }
    }

    if((current_segment - last_segment == 1) || (current_segment - last_segment == -3)) current_item++;
    else if((current_segment - last_segment == -1) || (current_segment - last_segment == 3))current_item--;

    last_segment = current_segment;

    if(current_item < 0) current_item = MAX_ITEMS -1;
    if(current_item >= MAX_ITEMS) current_item = 0;

    char buf[15];
    rb->snprintf(buf, sizeof(buf), "curseg: %d", current_segment);

    //draw text
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255)); //white
    rb->lcd_putsxy(x-radius, y+radius+10, buf);

    //draw segment lines
    rb->lcd_set_foreground(LCD_RGBPACK(0,0,255)); //blue
    rb->lcd_drawline(x-radius, y, x+radius, y);
    rb->lcd_drawline(x,y-radius,x,y+radius);

    rb->lcd_set_foreground(LCD_RGBPACK(255,0,255)); //purple
    fptp_drawcircle(x,y,radius);

    int x1 = x;
    int y1 = y;

    long sourcelen = sourcex*sourcex + sourcey*sourcey;
    sourcelen = fp_sqrt(sourcelen, 0); //find line length

    long mult = (((long)radius)<<14)/sourcelen;

    int x2 = (int)((sourcex*mult)>>14)+x1;
    int y2 = y1-(int)((sourcey*mult)>>14);

    if(tp_data_ptr->num_fingers == 1) rb->lcd_drawline(x1,y1,x2,y2);

    return;
}

void fptp_drawvwitems(int x, int y, int width, int height, int num, int selected)
{
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255)); //white

    int i;
    for(i=0; i<num; i++)
    {
        rb->lcd_drawrect(x,y+i*height,width,height);
        if(selected == i) rb->lcd_fillrect(x,y+i*height,width,height);
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter; //to avoid warnings

    tp_data_ptr = rb->fuzeplus_get_touchpad_data_ptr();

    last_segment = SEGMENT_INVALID;
    current_item = 0;

    middle_x = tp_data_ptr->max_x/2;
    middle_y = tp_data_ptr->max_y/2;

    while(1)
    {

        rb->button_clear_queue();
        rb->lcd_clear_display();

        fptp_drawdebugscreen(); //do a debug screen

        fptp_drawvirtualwheel(LCD_WIDTH-50,50,40,
                (tp_data_ptr->abs_x - middle_x),
                (tp_data_ptr->abs_y - middle_y));

        fptp_drawvwitems(LCD_WIDTH/2-20, 10, 20, 10, 10, current_item);

        rb->lcd_update();
        rb->sleep(1); //for some reason necessary

    }

    return PLUGIN_OK; //return.
}
