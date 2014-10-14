#include "plugin.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "sys.h"
#include "engine.h"
#include "pluginbitmaps/xworld_title.h"

#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
#define BTN_UP         BUTTON_LEFT
#define BTN_DOWN       BUTTON_RIGHT
#define BTN_LEFT       BUTTON_DOWN
#define BTN_RIGHT      BUTTON_UP
#define BTN_UP_LEFT    BUTTON_BOTTOMLEFT
#define BTN_UP_RIGHT   BUTTON_BACK
#define BTN_DOWN_LEFT  BUTTON_BOTTOMRIGHT
#define BTN_DOWN_RIGHT BUTTON_PLAYPAUSE
#define BTN_FIRE       BUTTON_VOL_UP
#define BTN_PAUSE      BUTTON_VOL_DOWN
#define BTN_MENU       BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define BTN_UP         BUTTON_UP
#define BTN_DOWN       BUTTON_DOWN
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_HOME
#define BTN_PAUSE      BUTTON_SELECT
#define BTN_MENU       BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define BTN_UP         BUTTON_UP
#define BTN_DOWN       BUTTON_DOWN
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_REC
#define BTN_PAUSE      BUTTON_SELECT
#define BTN_MENU       BUTTON_POWER

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) ||         \
    (CONFIG_KEYPAD == IPOD_3G_PAD) ||           \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BTN_UP         BUTTON_MENU
#define BTN_DOWN       BUTTON_PLAY
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      (BUTTON_MENU | BUTTON_SELECT)
#define BTN_MENU       (BUTTON_MENU | BUTTON_PLAY)

#else
#error Unsupported keypad
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void exit_handler(void)
{
    rb->cpu_boost(false);
}
#endif

static void do_title(void)
{
    debug(DBG_SYS, "Start do_title");
    rb->lcd_clear_display();
    rb->lcd_bitmap(xworld_title, 0,0, BMPWIDTH_xworld_title, BMPHEIGHT_xworld_title);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_setfont(FONT_SYSFIXED);
    int w;
    rb->lcd_getstringsize(TITLE_TEXT+1, &w, NULL);
    rb->lcd_putsxy(LCD_WIDTH/2 - .5 * w,
                   190,TITLE_TEXT);
    rb->lcd_update();
    int start_blur_tick=*rb->current_tick+TITLE_SHOW_TIME;
    while(TIME_BEFORE(*rb->current_tick, start_blur_tick))
    {
        if(rb->button_get(false)!=BUTTON_NONE)
            break;
        rb->yield();
    }
    /* do a downward blur */
    for(int i=0;i<TITLE_ANIM_STEPS;++i)
    {
        fb_data orig_fb[LCD_WIDTH*LCD_HEIGHT];
        rb->memcpy(orig_fb, rb->lcd_framebuffer, sizeof(orig_fb));
        for(int y=TITLE_ANIM_BLUR_LEVEL;y<LCD_HEIGHT-TITLE_ANIM_BLUR_LEVEL;++y)
        {
            for(int x=TITLE_ANIM_BLUR_LEVEL;x<LCD_WIDTH-TITLE_ANIM_BLUR_LEVEL;++x)
            {
                int red_sum=0, blue_sum=0, green_sum=0;
                for(int i=-TITLE_ANIM_BLUR_LEVEL;i<=TITLE_ANIM_BLUR_LEVEL-1;++i)
                    for(int j=-TITLE_ANIM_BLUR_LEVEL;j<=TITLE_ANIM_BLUR_LEVEL;++j)
                    {
                        red_sum+=RGB_UNPACK_RED(orig_fb[(y+i)*LCD_WIDTH+(x+j)]);
                        green_sum+=RGB_UNPACK_GREEN(orig_fb[(y+i)*LCD_WIDTH+(x+j)]);
                        blue_sum+=RGB_UNPACK_BLUE(orig_fb[(y+i)*LCD_WIDTH+(x+j)]);
                    }
                rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(red_sum/TITLE_ANIM_DIVISOR, green_sum/TITLE_ANIM_DIVISOR, blue_sum/TITLE_ANIM_DIVISOR);
            }
        }
        rb->lcd_update();
        /* sleep replaces yield here */
        rb->sleep(TITLE_ANIM_SLEEP);
    }
    rb->lcd_clear_display();
    debug(DBG_SYS, "End do_Title");
}

void sys_init(struct System* sys, const char* title)
{
    debug(DBG_SYS, "Start sys_init");

ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    rb_atexit(exit_handler);
#endif

    debug(DBG_SYS, "sys_init Point 0");
    /*do_title(); */

    debug(DBG_SYS, "sys_init Point 1");
    memset(&sys->input, 0, sizeof(sys->input));

    debug(DBG_SYS, "sys_init Point 2");
    sys->mutex_bitfield=~0;

    debug(DBG_SYS, "End sys_init");
}

void sys_destroy(struct System* sys)
{
    (void) sys;
    rb->splash(HZ, "Exiting...");
}

void sys_setPalette(struct System* sys, uint8_t start, uint8_t n, const uint8_t *buf)
{
    for(int i=start;i<start+n;++i)
    {
        uint8_t c[3];
        for (int j = 0; j < 3; j++) {
            uint8_t col = buf[i * 3 + j];
            c[j] = (col << 2) | (col & 3);
        }
        sys->pallete[i]=LCD_RGBPACK(c[0],c[1],c[2]);
    }
}

bool motion_blur_enabled=false;
bool antialias_enabled=false;
bool negative_enabled=false;

void sys_copyRect(struct System* sys, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buf, uint32_t pitch)
{
    debug(DBG_SYS, "sys is 0x%08x", sys);
    buf += y * pitch + x;
    while (h--) {
        /* one byte gives two pixels */
        for (int i = 0; i < w / 2; ++i) {
            uint8_t pix1=*(buf+i)>>4;
            uint8_t pix2=*(buf+i)&0xF;
#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
            rb->lcd_set_foreground(sys->pallete[pix1]);
            rb->lcd_drawpixel(LCD_WIDTH-(x+h), LCD_HEIGHT-(y+i*2));
            rb->lcd_set_foreground(sys->pallete[pix2]);
            rb->lcd_drawpixel(LCD_WIDTH-(x+h), LCD_HEIGHT-(y+i*2+1));
#else
            rb->lcd_set_foreground(sys->pallete[pix1]);
            rb->lcd_drawpixel(x+i*2, LCD_HEIGHT-(y+h));
            rb->lcd_set_foreground(sys->pallete[pix2]);
            rb->lcd_drawpixel(x+i*2+1, LCD_HEIGHT-(y+h));
#endif
        }
        buf += pitch;
    }
/* plugin buffer to small on target */

//    if(motion_blur_enabled)
//    {
//        static fb_data prev_frames[NUM_BLUR_FRAMES][LCD_WIDTH*LCD_HEIGHT];
//        static int last_frame=0;
//        rb->memmove(prev_frames[1], prev_frames[0], sizeof(prev_frames[0])*(NUM_BLUR_FRAMES-1));
//        rb->memcpy(prev_frames[0], rb->lcd_framebuffer, sizeof(prev_frames[0]));
//
//        if(last_frame<NUM_BLUR_FRAMES-1)
//            ++last_frame;
        /* blur in the last few frames */
//        for(int y=0;y<LCD_HEIGHT;++y)
//        {
//            for(int x=0;x<LCD_WIDTH;++x)
//            {
                /* give the last frame 1 weight, the next 2, 4, 8, so on */
//                int r_sum=0, g_sum=0, b_sum=0;
//                int weight=1;
//                for(int i=last_frame;i>0;--i,weight*=2)
//                {
//                    r_sum+=weight*RGB_UNPACK_RED(prev_frames[i][y*LCD_WIDTH+x]);
//                    g_sum+=weight*RGB_UNPACK_GREEN(prev_frames[i][y*LCD_WIDTH+x]);
//                    b_sum+=weight*RGB_UNPACK_BLUE(prev_frames[i][y*LCD_WIDTH+x]);
//                }
//                r_sum/=(float)(1<<(last_frame-1));
//                g_sum/=(float)(1<<(last_frame-1));
//                b_sum/=(float)(1<<(last_frame-1));
//                rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(r_sum, g_sum, b_sum);
//            }
//        }
//    }
//    if(antialias_enabled)
//    {
//        fb_data orig_fb[LCD_WIDTH*LCD_HEIGHT];
//        rb->memcpy(orig_fb, rb->lcd_framebuffer, sizeof(orig_fb));
//        for(int y=1;y<LCD_HEIGHT-1;++y)
//        {
//            for(int x=1;x<LCD_WIDTH-1;++x)
//            {
//                int red_sum=0, blue_sum=0, green_sum=0;
//                for(int i=-1;i<1;++i)
//                    for(int j=-1;j<1;++j)
//                    {
//                        red_sum+=RGB_UNPACK_RED(orig_fb[(y+i)*LCD_WIDTH+(x+j)]);
//                        green_sum+=RGB_UNPACK_GREEN(orig_fb[(y+i)*LCD_WIDTH+(x+j)]);
//                        blue_sum+=RGB_UNPACK_BLUE(orig_fb[(y+i)*LCD_WIDTH+(x+j)]);
//                    }
//                rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(red_sum/4, green_sum/4, blue_sum/4);
//            }
//        }
//    }
//    if(negative_enabled)
//    {
//        for(int y=0;y<LCD_HEIGHT;++y)
//        {
//            for(int x=0;x<LCD_WIDTH;++x)
//            {
//                int r,g,b;
//                fb_data pix=rb->lcd_framebuffer[y*LCD_WIDTH+x];
//                r=RGB_UNPACK_RED(pix);
//                g=RGB_UNPACK_GREEN(pix);
//                b=RGB_UNPACK_BLUE(pix);
//                rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(0xff-r, 0xff-g, 0xff-b);
//            }
//        }
//    }
    rb->lcd_update();
}

static void do_filter_settings(void)
{
    MENUITEM_STRINGLIST(menu, "Filter Settings", NULL,
                        "Motion Blur",
                        "Antialiasing",
                        "Negative",
                        "Back");
    int sel=0;
    while(1)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->set_bool("Motion Blur", &motion_blur_enabled);
            break;
        case 1:
            rb->set_bool("Antialiasing", &antialias_enabled);
            break;
        case 2:
            rb->set_bool("Negative", &negative_enabled);
            break;
        case 3:
            rb->lcd_clear_display();
            return;
        }
    }
}

static void do_pause_menu(struct System* sys)
{

    int sel=0;
    MENUITEM_STRINGLIST(menu,"XWorld Menu", NULL,
                        "Resume Game",
                        "Reset Game",
                        "Playback Control",
                        "Filter Settings",
                        "Quit");

    bool quit=false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            return;
        case 1:
            engine_init(&sys->e);
            return;
        case 2:
            playback_control(NULL);
            break;
        case 3:
            do_filter_settings();
            break;
        case 5:
            exit(1);
            break;
        }
    }
}

void sys_processEvents(struct System* sys)
{
    int btn=rb->button_get(false);
    btn &= ~BUTTON_REDRAW;
    debug(DBG_SYS, "button is 0x%08x", btn);
    if(btn==BUTTON_NONE || btn & 0x80000000)
        return;
    if(btn & BUTTON_REL)
    {
        debug(DBG_SYS, "button_rel");
        btn &= ~BUTTON_REL;

        if(btn & BTN_FIRE)
            sys->input.button=false;
        if(btn & BTN_UP)
            sys->input.dirMask &= ~DIR_UP;
        if(btn & BTN_DOWN)
            sys->input.dirMask &= ~DIR_DOWN;
        if(btn & BTN_LEFT)
            sys->input.dirMask &= ~DIR_LEFT;
        if(btn & BTN_RIGHT)
            sys->input.dirMask &= ~DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(btn & BTN_DOWN_LEFT)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(btn & BTN_DOWN_RIGHT)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(btn & BTN_UP_LEFT)
            sys->input.dirMask &= ~(DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(btn & BTN_UP_RIGHT)
            sys->input.dirMask &= ~(DIR_UP | DIR_RIGHT);
#endif

    }
        else
        {
        if(btn & BTN_FIRE)
            sys->input.button=true;
        if(btn & BTN_UP)
            sys->input.dirMask |= DIR_UP;
        if(btn & BTN_DOWN)
            sys->input.dirMask |= DIR_DOWN;
        if(btn & BTN_LEFT)
            sys->input.dirMask |= DIR_LEFT;
        if(btn & BTN_RIGHT)
            sys->input.dirMask |= DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(btn & BTN_DOWN_LEFT)
            sys->input.dirMask |= (DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(btn & BTN_DOWN_RIGHT)
            sys->input.dirMask |= (DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(btn & BTN_UP_LEFT)
            sys->input.dirMask |= (DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(btn & BTN_UP_RIGHT)
            sys->input.dirMask |= (DIR_UP | DIR_RIGHT);
#endif
        switch(btn)
        {
        case BTN_PAUSE:
            do_pause_menu(sys);
            break;
        case BTN_MENU:
            sys->input.code=true;
            break;
        }
    }
    debug(DBG_SYS, "dirMask is 0x%02x", sys->input.dirMask);
    debug(DBG_SYS, "button is %s", sys->input.button==true?"true":"false");
}

void sys_sleep(struct System* sys, uint32_t duration)
{
    (void) sys;
    rb->sleep(duration/10);
}

uint32_t sys_getTimeStamp(struct System* sys)
{
    (void) sys;
    return (uint32_t) *rb->current_tick;
}

void sys_startAudio(struct System* sys, AudioCallback callback, void *param)
{
}

void sys_stopAudio(struct System* sys)
{
}

uint32_t sys_getOutputSampleRate(struct System* sys)
{
    (void) sys;
    return 44100;
}

void *sys_addTimer(struct System* sys, uint32_t delay, TimerCallback callback, void *param)
{
}

void sys_removeTimer(struct System* sys, void *timerId)
{
}

void *sys_createMutex(struct System* sys)
{
    if(!sys)
        error("sys is NULL!");
    for(int i=0;i<MAX_MUTEXES;++i)
    {
        if(sys->mutex_bitfield & (1<<i))
        {
            rb->mutex_init(&sys->mutex_memory[i]);
            sys->mutex_bitfield |= (1 << i);
            return &sys->mutex_memory[i];
        }
    }
    warning("Out of mutexes!");
    return NULL;
}

void sys_destroyMutex(struct System* sys, void *mutex)
{
    int mutex_number=(mutex-(void*)sys->mutex_memory)/sizeof(struct mutex); /* pointer arithmetic! check for bugs! */
    sys->mutex_bitfield &= ~(1 << mutex_number);
}

void sys_lockMutex(struct System* sys, void *mutex)
{
    (void) sys;
    rb->mutex_lock((struct mutex*) mutex);
}

void sys_unlockMutex(struct System* sys, void *mutex)
{
    (void) sys;
    rb->mutex_unlock((struct mutex*) mutex);
}

uint8_t* getOffScreenFramebuffer(struct System* sys)
{
    (void) sys;
    return NULL;
}

void MutexStack(struct MutexStack_t* s, struct System *stub, void *mutex)
{
    s->sys=stub;
    s->_mutex=mutex;
    sys_lockMutex(s->sys, s->_mutex);
}

void MutexStack_destroy(struct MutexStack_t* s)
{
    sys_unlockMutex(s->sys, s->_mutex);
}
