#include "plugin.h"
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_bmp.h"
#include "lib/pluginlib_exit.h"
#include "sys.h"
#include "parts.h"
#include "engine.h"
#include "keymaps.h"

static struct System* save_sys;

static bool sys_save_settings(struct System* sys)
{
    File f;
    file_create(&f, false);
    if(!file_open(&f, SETTINGS_FILE, sys->e->_saveDir, "wb"))
    {
        return false;
    }
    file_write(&f, &sys->settings, sizeof(sys->settings));
    file_close(&f);
    return true;
}

static void sys_rotate_keymap(struct System* sys)
{
    switch(sys->settings.rotation_option)
    {
    case 0:
        sys->keymap.up=BTN_UP;
        sys->keymap.down=BTN_DOWN;
        sys->keymap.left=BTN_LEFT;
        sys->keymap.right=BTN_RIGHT;
#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
        sys->keymap.upleft=BTN_UP_LEFT;
        sys->keymap.upright=BTN_UP_RIGHT;
        sys->keymap.downleft=BTN_DOWN_RIGHT;
        sys->keymap.downright=BTN_DOWN_LEFT;
#endif
        break;
    case 1:
        sys->keymap.up=BTN_RIGHT;
        sys->keymap.down=BTN_LEFT;
        sys->keymap.left=BTN_UP;
        sys->keymap.right=BTN_DOWN;
#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
        sys->keymap.upleft=BTN_UP_RIGHT;
        sys->keymap.upright=BTN_DOWN_RIGHT;
        sys->keymap.downleft=BTN_UP_LEFT;
        sys->keymap.downright=BTN_DOWN_LEFT;
#endif
        break;
    case 2:
        sys->keymap.up=BTN_LEFT;
        sys->keymap.down=BTN_RIGHT;
        sys->keymap.left=BTN_DOWN;
        sys->keymap.right=BTN_UP;
#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
        sys->keymap.upleft=BTN_DOWN_LEFT;
        sys->keymap.upright=BTN_UP_LEFT;
        sys->keymap.downleft=BTN_DOWN_RIGHT;
        sys->keymap.downright=BTN_DOWN_LEFT;
#endif
        break;
    default:
        error("sys_rotate_keymap: fall-through!");
    }
}

static bool sys_load_settings(struct System* sys)
{
    File f;
    file_create(&f, false);
    if(!file_open(&f, SETTINGS_FILE, sys->e->_saveDir, "rb"))
    {
        return false;
    }
    file_read(&f, &sys->settings, sizeof(sys->settings));
    file_close(&f);
    sys_rotate_keymap(sys);
    return true;
}

void exit_handler(void)
{
    sys_save_settings(save_sys);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();
}

static bool sys_do_help(void)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_setfont(FONT_UI);
    char* help_text[]= {"XWorld", "",
                        "XWorld", "is", "an", "interpreter", "for", "Another", "World,", "a", "fantastic", "game", "by", "Eric", "Chahi."
    };
    struct style_text style[]={
        {0, TEXT_CENTER|TEXT_UNDERLINE},
        LAST_STYLE_ITEM
    };
    return display_text(ARRAYLEN(help_text), help_text, style,NULL,true);
}

static const struct opt_items scaling_settings[3] = {
    { "Disabled", -1 },
    { "Fast"    , -1 },
#ifdef HAVE_LCD_COLOR
    { "Good"    , -1 }
#endif
};

static const struct opt_items rotation_settings[3] = {
    { "Disabled", -1 },
    { "Clockwise"    , -1 },
    { "Anticlockwise"    , -1 }
};

static void do_video_settings(struct System* sys)
{
    MENUITEM_STRINGLIST(menu, "Video Settings", NULL,
                        "Motion Blur",
                        "Antialiasing",
                        "Negative",
                        "Scaling",
                        "Rotation",
                        "Show FPS",
                        "Back");
    int sel=0;
    while(1)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->set_bool("Motion Blur", &sys->settings.motion_blur_enabled);
            break;
        case 1:
            rb->set_bool("Antialiasing", &sys->settings.antialias_enabled);
            break;
        case 2:
            rb->set_bool("Negative", &sys->settings.negative_enabled);
            break;
        case 3:
            rb->set_option("Scaling", &sys->settings.scaling_option, INT, scaling_settings,
#ifdef HAVE_LCD_COLOR
                           3
#else
                           2
#endif
                           , NULL);
            break;
        case 4:
            rb->set_option("Rotation", &sys->settings.rotation_option, INT, rotation_settings, 3, NULL);
            sys_rotate_keymap(sys);
            break;
        case 5:
            rb->set_bool("Show FPS", &sys->settings.showfps);
            break;
        case 6:
            rb->lcd_clear_display();
            return;
        }
    }
}

static void sys_reset_settings(struct System* sys)
{
    sys->settings.motion_blur_enabled=false;
    sys->settings.antialias_enabled=false;
    sys->settings.negative_enabled=false;
    sys->settings.rotation_option=0;
    sys->settings.scaling_option=1;
    sys->settings.showfps=true;
    sys_rotate_keymap(sys);
}

static struct System* mainmenu_sysptr;

static int mainmenu_cb(int action, const struct menu_item_ex *this_item)
{
    int idx=((intptr_t)this_item);
    if(action==ACTION_REQUEST_MENUITEM && !mainmenu_sysptr->loaded && (idx==0 || idx==8 || idx==10))
        return ACTION_EXIT_MENUITEM;
    return action;
}

void sys_menu(struct System* sys)
{
    rb->splash(0, "Loading...");
    sys->loaded=engine_loadGameState(sys->e, 0);
    rb->lcd_update();
    mainmenu_sysptr=sys;
    int sel=0;
    MENUITEM_STRINGLIST(menu, "XWorld Menu", mainmenu_cb,
                        "Resume Game",          /* 0  */
                        "Start New Game",       /* 1  */
                        "Playback Control",     /* 2  */
                        "Video Settings",       /* 3  */
                        "Fast Mode",            /* 4  */
                        "Help",                 /* 5  */
                        "Reset Settings",       /* 6  */
                        "Load",                 /* 7  */
                        "Save",                 /* 8  */
                        "Quit without Saving",  /* 9  */
                        "Save and Quit");       /* 10 */
    bool quit=false;
    while(!quit)
    {
        int item;
        switch(item = rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            return;
        case 1:
            vm_initForPart(&sys->e->vm, GAME_PART_FIRST); // This game part is the protection screen
            return;
        case 2:
            playback_control(NULL);
            break;
        case 3:
            do_video_settings(sys);
            break;
        case 4:
            rb->set_bool("Fast Mode", &sys->e->vm._fastMode);
            break;
        case 5:
            sys_do_help();
            break;
        case 6:
            sys_reset_settings(sys);
            break;
        case 7:
            rb->splash(0, "Loading...");
            sys->loaded=engine_loadGameState(sys->e, 0);
            rb->lcd_update();
            break;
        case 8:
            sys->e->_stateSlot=0;
            rb->splash(0, "Saving...");
            engine_saveGameState(sys->e, sys->e->_stateSlot, "quicksave");
            rb->lcd_update();
            break;
        case 9:
            engine_deleteGameState(sys->e, 0);
            exit(PLUGIN_OK);
            break;
        case 10:
            /* saves are NOT deleted on loading */
            exit(PLUGIN_OK);
        default:
            error("sys_menu: fall-through!");
        }
    }
}

void sys_init(struct System* sys, const char* title)
{
    (void) title;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    backlight_ignore_timeout();
    rb_atexit(exit_handler);
    save_sys=sys;
    rb->memset(&sys->input, 0, sizeof(sys->input));
    sys->mutex_bitfield=~0;
    if(!sys_load_settings(sys))
    {
        sys_reset_settings(sys);
    }
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
#if (LCD_DEPTH > 16) && (LCD_DEPTH <= 24)
        sys->pallete[i]=(fb_data) { c[2], c[1], c[0] };
#elif defined(HAVE_LCD_COLOR)
        sys->pallete[i]=FB_RGBPACK(c[0],c[1],c[2]);
#elif LCD_DEPTH > 1
        sys->pallete[i]=FB_BRIGHTNESS((c[0]+c[1]+c[2])/3);
#endif
    }
}

void sys_copyRect(struct System* sys, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buf, uint32_t pitch)
{
    static int last_timestamp=1;
    fb_data* framebuffer=(fb_data*) sys->e->res._memPtrStart + MEM_BLOCK_SIZE + (4 * VID_PAGE_SIZE);
    buf += y * pitch + x;
    if(sys->settings.rotation_option)
    {
        /* clockwise */
        if(sys->settings.rotation_option == 1)
        {
            while (h--) {
                /* one byte gives two pixels */
                for (int i = 0; i < w / 2; ++i) {
                    uint8_t pix1=*(buf+i)>>4;
                    uint8_t pix2=*(buf+i)&0xF;
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
                    framebuffer[( (h * 2)    ) * 320 + i] = sys->pallete[pix1];
                    framebuffer[( (h * 2) + 1) * 320 + i] = sys->pallete[pix2];
#else
                    framebuffer[( (i * 2)    ) * 200 + h] = sys->pallete[pix1];
                    framebuffer[( (i * 2) + 1) * 200 + h] = sys->pallete[pix2];
#endif
                }
                buf += pitch;
            }
        }
        /* counterclockwise */
        else
        {
            while (h--) {
                /* one byte gives two pixels */
                for (int i = 0; i < w / 2; ++i) {
                    uint8_t pix1=*(buf+i)>>4;
                    uint8_t pix2=*(buf+i)&0xF;
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
                    framebuffer[(200 - h * 2    ) * 320 + ( 320 - i )] = sys->pallete[pix1];
                    framebuffer[(200 - h * 2 - 1) * 320 + ( 320 - i )] = sys->pallete[pix2];
#else
                    framebuffer[(320 - i * 2    ) * 200 + ( 200 - h )] = sys->pallete[pix1];
                    framebuffer[(320 - i * 2 - 1) * 200 + ( 200 - h )] = sys->pallete[pix2];
#endif
                }
                buf += pitch;
            }
        }
    }
    /* no rotation */
    else
    {
        int next=0;

#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
        /* one byte gives two pixels */
        //w/=2;
        for(int j=0;j<h;++j)
        {
            for(int i=0;i<w/2;++i)
            {
                uint8_t pix1=*(buf+i)>>4;
                uint8_t pix2=*(buf+i)&0xF;
                framebuffer[i*200+j]=sys->pallete[pix1];
                ++next;
                framebuffer[(i+1)*200+j]=sys->pallete[pix2];
                ++next;
            }
            buf += pitch;
        }
#else
        while (h--) {
            /* one byte gives two pixels */
            for (int i = 0; i < w / 2; ++i) {
                uint8_t pix1=*(buf+i)>>4;
                uint8_t pix2=*(buf+i)&0xF;
                framebuffer[next]=sys->pallete[pix1];
                ++next;
                framebuffer[next]=sys->pallete[pix2];
                ++next;
            }
            buf += pitch;
        }
#endif
    }
    if(sys->settings.scaling_option)
    {
        struct bitmap in_bmp;
        if(sys->settings.rotation_option)
        {
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
            in_bmp.width=320;
            in_bmp.height=200;
#else
            in_bmp.width=200;
            in_bmp.height=320;
#endif
        }
        else
        {
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
            in_bmp.width=200;
            in_bmp.height=320;
#else
            in_bmp.width=320;
            in_bmp.height=200;
#endif
        }
        in_bmp.data=(unsigned char*) framebuffer;
        struct bitmap out_bmp;
        out_bmp.width=LCD_WIDTH;
        out_bmp.height=LCD_HEIGHT;
        out_bmp.data=(unsigned char*) rb->lcd_framebuffer;

#ifdef HAVE_LCD_COLOR
        if(sys->settings.scaling_option==1)
#endif
            simple_resize_bitmap(&in_bmp, &out_bmp);
#ifdef HAVE_LCD_COLOR
        else
            smooth_resize_bitmap(&in_bmp, &out_bmp);
#endif
    }
    else
    {
        if(sys->settings.rotation_option)
            rb->lcd_bitmap(framebuffer, 0, 0, 200, 320);
        else
            rb->lcd_bitmap(framebuffer, 0, 0, 320, 200);
    }

    /* Moving these to the audio buffer as well, may work around */

    /* plugin buffer to small on target */

    /* if(motion_blur_enabled) */
    /* { */
    /*     static fb_data prev_frames[NUM_BLUR_FRAMES][LCD_WIDTH*LCD_HEIGHT]; */
    /*     static int last_frame=0; */
    /*     rb->memmove(prev_frames[1], prev_frames[0], sizeof(prev_frames[0])*(NUM_BLUR_FRAMES-1)); */
    /*     rb->memcpy(prev_frames[0], rb->lcd_framebuffer, sizeof(prev_frames[0])); */

    /*     if(last_frame<NUM_BLUR_FRAMES-1) */
    /*         ++last_frame; */
    /*     /\* blur in the last few frames *\/ */
    /*     for(int y=0;y<LCD_HEIGHT;++y) */
    /*     { */
    /*         for(int x=0;x<LCD_WIDTH;++x) */
    /*         { */
    /*             /\* give the last frame 1 weight, the next 2, 4, 8, so on *\/ */
    /*             int r_sum=0, g_sum=0, b_sum=0; */
    /*             int weight=1; */
    /*             for(int i=last_frame;i>0;--i,weight*=2) */
    /*             { */
    /*                 r_sum+=weight*RGB_UNPACK_RED(prev_frames[i][y*LCD_WIDTH+x]); */
    /*                 g_sum+=weight*RGB_UNPACK_GREEN(prev_frames[i][y*LCD_WIDTH+x]); */
    /*                 b_sum+=weight*RGB_UNPACK_BLUE(prev_frames[i][y*LCD_WIDTH+x]); */
    /*             } */
    /*             r_sum/=(float)(1<<(last_frame-1)); */
    /*             g_sum/=(float)(1<<(last_frame-1)); */
    /*             b_sum/=(float)(1<<(last_frame-1)); */
    /*             rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(r_sum, g_sum, b_sum); */
    /*         } */
    /*     } */
    /* } */
    /* if(antialias_enabled) */
    /* { */
    /*     fb_data orig_fb[LCD_WIDTH*LCD_HEIGHT]; */
    /*     rb->memcpy(orig_fb, rb->lcd_framebuffer, sizeof(orig_fb)); */
    /*     for(int y=1;y<LCD_HEIGHT-1;++y) */
    /*     { */
    /*         for(int x=1;x<LCD_WIDTH-1;++x) */
    /*         { */
    /*             int red_sum=0, blue_sum=0, green_sum=0; */
    /*             for(int i=-1;i<1;++i) */
    /*                 for(int j=-1;j<1;++j) */
    /*                 { */
    /*                     red_sum+=RGB_UNPACK_RED(orig_fb[(y+i)*LCD_WIDTH+(x+j)]); */
    /*                     green_sum+=RGB_UNPACK_GREEN(orig_fb[(y+i)*LCD_WIDTH+(x+j)]); */
    /*                     blue_sum+=RGB_UNPACK_BLUE(orig_fb[(y+i)*LCD_WIDTH+(x+j)]); */
    /*                 } */
    /*             rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(red_sum/4, green_sum/4, blue_sum/4); */
    /*         } */
    /*     } */
    /* } */
    if(sys->settings.negative_enabled)
    {
        for(int y=0;y<LCD_HEIGHT;++y)
        {
            for(int x=0;x<LCD_WIDTH;++x)
            {
                int r,g,b;
                fb_data pix=rb->lcd_framebuffer[y*LCD_WIDTH+x];
                r=RGB_UNPACK_RED(pix);
                g=RGB_UNPACK_GREEN(pix);
                b=RGB_UNPACK_BLUE(pix);
                rb->lcd_framebuffer[y*LCD_WIDTH+x]=LCD_RGBPACK(0xff-r, 0xff-g, 0xff-b);
            }
        }
    }
    int current_time=sys_getTimeStamp(sys);
    if(sys->settings.showfps)
    {
        /* use 1000 and not HZ here, getTimeStamp is in milliseconds */
        rb->lcd_putsf(0,0,"FPS: %d", 1000/((current_time-last_timestamp==0)?1:current_time-last_timestamp));
    }
    rb->lcd_update();
    last_timestamp=sys_getTimeStamp(sys);
}

static void do_pause_menu(struct System* sys)
{
    int sel=0;
    MENUITEM_STRINGLIST(menu,"XWorld Menu", NULL,
                        "Resume Game",         /* 0 */
                        "Start New Game",      /* 1 */
                        "Playback Control",    /* 2 */
                        "Video Settings",      /* 3 */
                        "Fast Mode",           /* 4 */
                        "Enter Code",          /* 5 */
                        "Help",                /* 6 */
                        "Reset Settings",      /* 7 */
                        "Load",                /* 8 */
                        "Save",                /* 9 */
                        "Quit");               /* 10 */

    bool quit=false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->lcd_clear_display();
            return;
        case 1:
            /* This calls sys_init which resets the settings, which is very anouying, it works so far with out it */
            //engine_init(sys->e);
            vm_initForPart(&sys->e->vm, GAME_PART_FIRST);
            return;
        case 2:
            playback_control(NULL);
            break;
        case 3:
            do_video_settings(sys);
            break;
        case 4:
            rb->set_bool("Fast Mode", &sys->e->vm._fastMode);
            break;
        case 5:
            sys->input.code=true;
            rb->lcd_clear_display();
            return;
        case 6:
            sys_do_help();
            break;
        case 7:
            sys_reset_settings(sys);
            break;
        case 8:
            rb->splash(0, "Loading...");
            sys->loaded=engine_loadGameState(sys->e, 0);
            rb->lcd_update();
            return;
        case 9:
            sys->e->_stateSlot=0;
            rb->splash(0, "Saving...");
            engine_saveGameState(sys->e, sys->e->_stateSlot, "quicksave");
            rb->lcd_update();
            break;
        case 10:
            exit(PLUGIN_OK);
            break;
        }
    }
}

void sys_processEvents(struct System* sys)
{
    int btn=rb->button_get(false);
    btn &= ~BUTTON_REDRAW;
    debug(DBG_SYS, "button is 0x%08x", btn);

    /* exit early if we can */
    if(btn == BUTTON_NONE)
    {
        return;
    }

    /* Ignore some buttons that cause errant input */
#if (CONFIG_KEYPAD == (IPOD_4G_PAD || IPOD_3G_PAD || IPOD_1G2G_PAD))
    if(btn & 0x80000000)
        return;
#endif
#if (CONFIG_KEYPAD == SANSA_E200_PAD)
    if(btn==(BUTTON_SCROLL_FWD || BUTTON_SCROLL_BACK))
        return;
#endif
#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
    if(btn==(BUTTON_SELECT))
        return;
#endif

    /* handle special keys first */
    switch(btn)
    {
    case BTN_PAUSE:
        do_pause_menu(sys);
        sys->input.dirMask=0;
        /* exit early to avoid unwanted button presses being detected */
        return;
    default:
        exit_on_usb(btn);
        break;
    }

    /* handle releases */
    if(btn & BUTTON_REL)
    {
        debug(DBG_SYS, "button_rel");
        btn &= ~BUTTON_REL;

        if(btn & BTN_FIRE)
            sys->input.button=false;
        if(btn & sys->keymap.up)
            sys->input.dirMask &= ~DIR_UP;
        if(btn & sys->keymap.down)
            sys->input.dirMask &= ~DIR_DOWN;
        if(btn & sys->keymap.left)
            sys->input.dirMask &= ~DIR_LEFT;
        if(btn & sys->keymap.right)
            sys->input.dirMask &= ~DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(btn & sys->keymap.downleft)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(btn & sys->keymap.downright)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(btn & sys->keymap.upleft)
            sys->input.dirMask &= ~(DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(btn & sys->keymap.upright)
            sys->input.dirMask &= ~(DIR_UP | DIR_RIGHT);
#endif
    }
    /* then handle presses */
    else
    {
        if(btn & BTN_FIRE)
            sys->input.button=true;
        if(btn & sys->keymap.up)
            sys->input.dirMask |= DIR_UP;
        if(btn & sys->keymap.down)
            sys->input.dirMask |= DIR_DOWN;
        if(btn & sys->keymap.left)
            sys->input.dirMask |= DIR_LEFT;
        if(btn & sys->keymap.right)
            sys->input.dirMask |= DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(btn & sys->keymap.downleft)
            sys->input.dirMask |= (DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(btn & sys->keymap.downright)
            sys->input.dirMask |= (DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(btn & sys->keymap.upleft)
            sys->input.dirMask |= (DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(btn & sys->keymap.upright)
            sys->input.dirMask |= (DIR_UP | DIR_RIGHT);
#endif
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
    return (uint32_t) (*rb->current_tick)*10;
}

#define NBSAMPLES 32

static int16_t rb_soundbuf[NBSAMPLES] IBSS_ATTR;
static int8_t temp_soundbuf[NBSAMPLES] IBSS_ATTR;
static AudioCallback audio_callback;
static void* audio_param;
static void get_more(const void** start, size_t* size)
{
    audio_callback(audio_param, temp_soundbuf, NBSAMPLES);
    /* convert xworld format (signed 8-bit) to rockbox format (signed 16-bit) */
    for(int i=0;i<NBSAMPLES;++i)
    {
        rb_soundbuf[i]=temp_soundbuf[i]*0x100;
    }
    *start=rb_soundbuf;
    *size=NBSAMPLES;
}

void sys_startAudio(struct System* sys, AudioCallback callback, void *param)
{
    (void) sys;
    audio_callback=callback;
    audio_param=param;
    rb->pcm_play_data(get_more, NULL, NULL, 0);
}

void sys_stopAudio(struct System* sys)
{
    (void) sys;
    rb->audio_stop();
}

uint32_t sys_getOutputSampleRate(struct System* sys)
{
    (void) sys;
    return rb->mixer_get_frequency();
}

/* pretty non-reentrant here, but who cares? it's not like someone
   would want to run two instances of the game on Rockbox! :D */

static uint32_t cur_delay;
static void* cur_param;
static TimerCallback user_callback;
static void timer_callback(void)
{
    debug(DBG_SYS, "timer callback with delay of %d ms callback 0x%08x param 0x%08x", cur_delay, timer_callback, cur_param);
    uint32_t ret = user_callback(cur_delay, cur_param);
    if(ret!=cur_delay)
    {
        cur_delay=ret;
        rb->timer_register(1, NULL, TIMER_FREQ/1000*ret, timer_callback IF_COP(, CPU));
    }
}

void *sys_addTimer(struct System* sys, uint32_t delay, TimerCallback callback, void *param)
{
    (void) sys;
    debug(DBG_SYS, "registering timer with delay of %d ms callback 0x%08x param 0x%08x", delay, callback, param);
    user_callback=callback;
    cur_delay=delay;
    cur_param=param;
    rb->timer_register(1, NULL, TIMER_FREQ/1000*delay, timer_callback IF_COP(, CPU));
    return NULL;
}

void sys_removeTimer(struct System* sys, void *timerId)
{
    (void) sys;
    (void) timerId;
    /* there's only one timer per game instance, so ignore both parameters */
    rb->timer_unregister();
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
    debug(DBG_SYS, "calling mutex_lock");
    rb->mutex_lock((struct mutex*) mutex);
}

void sys_unlockMutex(struct System* sys, void *mutex)
{
    (void) sys;
    debug(DBG_SYS, "calling mutex_unlock");
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
