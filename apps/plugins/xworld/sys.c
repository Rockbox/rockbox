#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "sys.h"
void sys_init(struct System* sys, const char* title)
{
    rb->splash(HZ, title);
    memset(&sys->input, 0, sizeof(sys->input));
    sys->mutex_bitfield=0;
}
void sys_destroy(struct System* sys)
{
    (void) sys;
    rb->splash(HZ, "Exiting...");
}
void sys_setPallete(struct System* sys, uint8_t start, uint8_t n, const uint8_t *buf)
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
    (void) sys;
    buf += y * pitch + x;
    while (h--) {
        /* one byte gives two pixels */
        for (int i = 0; i < w / 2; ++i) {
            uint8_t pix1=*(buf+i)>>4;
            uint8_t pix2=*(buf+i)&0xF;
            rb->lcd_set_foreground(sys->pallete[pix1]);
            rb->lcd_drawpixel(x+i*2, y+h);
            rb->lcd_set_foreground(sys->pallete[pix2]);
            rb->lcd_drawpixel(x+i*2+1, y+h);
        }
        buf += pitch;
    }
    rb->lcd_update();
}

void sys_processEvents(struct System* sys)
{
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int button=pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));
    if(button&PLA_UP)
    {
        sys->input.dirMask|=DIR_UP;
    }
    if(button&PLA_DOWN)
    {
        sys->input.dirMask|=DIR_DOWN;
    }
    if(button&PLA_LEFT)
    {
        sys->input.dirMask|=DIR_LEFT;
    }
    if(button&PLA_RIGHT)
    {
        sys->input.dirMask|=DIR_RIGHT;
    }
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
