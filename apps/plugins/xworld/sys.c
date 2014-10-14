#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "sys.h"

#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
#define BTN_UP         BUTTON_UP
#define BTN_DOWN       BUTTON_DOWN
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_UP_LEFT    BUTTON_BACK
#define BTN_UP_RIGHT   BUTTON_PLAYPAUSE
#define BTN_DOWN_LEFT  BUTTON_BOTTOMLEFT
#define BTN_DOWN_RIGHT BUTTON_BOTTOMRIGHT
#define BTN_FIRE       BUTTON_VOL_DOWN
#define BTN_PAUSE      BUTTON_VOL_UP
#define BTN_MENU       BUTTON_POWER

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
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

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BTN_UP         BUTTON_MENU
#define BTN_DOWN       BUTTON_PLAY
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      BUTTON_SCROLL_BACK
#define BTN_MENU       BUTTON_SCROLL_FWD

#else
#error Unsupported keypad
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void exit_handler(void)
{
    rb->cpu_boost(false);
}
#endif

void sys_init(struct System* sys, const char* title)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    atexit(exit_handler);
#endif
    rb->splash(HZ, title);
    memset(&sys->input, 0, sizeof(sys->input));
    sys->mutex_bitfield=0xFFFF;
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
void sys_copyRect(struct System* sys, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buf, uint32_t pitch)
{
    debug(DBG_SYS, "sys is 0x%08x", sys);
    buf += y * pitch + x;
    while (h--) {
        /* one byte gives two pixels */
        for (int i = 0; i < w / 2; ++i) {
            uint8_t pix1=*(buf+i)>>4;
            uint8_t pix2=*(buf+i)&0xF;
            rb->lcd_set_foreground(sys->pallete[pix1]);
            rb->lcd_drawpixel(x+i*2, LCD_HEIGHT-(y+h));
            rb->lcd_set_foreground(sys->pallete[pix2]);
            rb->lcd_drawpixel(x+i*2+1, LCD_HEIGHT-(y+h));
        }
        buf += pitch;
    }
    rb->lcd_update();
}

void sys_processEvents(struct System* sys)
{
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int btn=pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));
    debug(DBG_SYS, "button is 0x%08x", btn);
    if(btn & BUTTON_REL)
    {
        debug(DBG_SYS, "button_rel");
        switch(btn & ~BUTTON_REL)
        {
        case BTN_FIRE:
            sys->input.button=false;
            break;
        case BTN_LEFT:
            sys->input.dirMask &= ~DIR_LEFT;
            break;
        case BTN_RIGHT:
            sys->input.dirMask &= ~DIR_RIGHT;
            break;
        case BTN_UP:
            sys->input.dirMask &= ~DIR_UP;
            break;
        case BTN_DOWN:
            sys->input.dirMask &= ~DIR_DOWN;
            break;
        #ifdef  BTN_DOWN_LEFT
            case BTN_DOWN_LEFT:
                sys->input.dirMask &= (~DIR_LEFT | ~DIR_DOWN);
                break;
        #endif
        #ifdef  BTN_DOWN_RIGHT
            case BTN_DOWN_RIGHT:
                sys->input.dirMask &= (~DIR_RIGHT | ~DIR_DOWN);
                break;
        #endif
        #ifdef  BTN_UP_LEFT
            case BTN_UP_LEFT:
                sys->input.dirMask &= (~DIR_LEFT | ~DIR_UP);
                break;
        #endif
        #ifdef  BTN_UP_RIGHT
            case BTN_UP_RIGHT:
                sys->input.dirMask &= (~DIR_RIGHT | ~DIR_UP);
                break;
        #endif
        }
    }
    else
    {
        switch(btn)
        {
        case BTN_FIRE:
            sys->input.button=true;
            break;
        case BTN_FIRE | BUTTON_REL:
            sys->input.button=false;
            break;
        case BTN_LEFT:
            sys->input.dirMask |= DIR_LEFT;
            break;
        case BTN_LEFT | BUTTON_REL:
            sys->input.dirMask &= ~DIR_LEFT;
            break;
        case BTN_RIGHT:
            sys->input.dirMask |= DIR_RIGHT;
            break;
        case BTN_RIGHT | BUTTON_REL:
            sys->input.dirMask &= ~DIR_RIGHT;
            break;
        case BTN_UP:
            sys->input.dirMask |= DIR_UP;
            break;
        case BTN_UP | BUTTON_REL:
            sys->input.dirMask &= ~DIR_UP;
            break;
        case BTN_DOWN:
            sys->input.dirMask |= DIR_DOWN;
            break;
        case BTN_DOWN | BUTTON_REL:
            sys->input.dirMask &= ~DIR_DOWN;
            break;
        #ifdef BTN_DOWN_LEFT
            case BTN_DOWN_LEFT:
                sys->input.dirMask |= (DIR_LEFT | DIR_DOWN);
                break;
            case BTN_DOWN_LEFT | BUTTON_REL:
                sys->input.dirMask &= (~DIR_LEFT | ~DIR_DOWN);
                break;
        #endif
        #ifdef BTN_DOWN_RIGHT
            case BTN_DOWN_RIGHT:
                sys->input.dirMask |= (DIR_RIGHT | DIR_DOWN);
                break;
            case BTN_DOWN_RIGHT | BUTTON_REL:
                sys->input.dirMask &= (~DIR_RIGHT | ~DIR_DOWN);
                break;
        #endif
        #ifdef BTN_UP_LEFT
            case BTN_UP_LEFT:
                sys->input.dirMask |= (DIR_LEFT | DIR_UP);
                break;
            case BTN_UP_LEFT | BUTTON_REL:
                sys->input.dirMask &= (~DIR_LEFT | ~DIR_UP);
                break;
        #endif
        #ifdef BTN_UP_RIGHT
            case BTN_UP_RIGHT:
                sys->input.dirMask |= (DIR_RIGHT | DIR_UP);
                break;
            case BTN_UP_RIGHT | BUTTON_REL:
                sys->input.dirMask &= (~DIR_RIGHT| ~DIR_UP);
                break;
        #endif
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
