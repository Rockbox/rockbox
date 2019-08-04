#define NO_MMSUPP_DEFINES

#include "plugin.h"
#include "lib/configfile.h"
#include "mikmod.h"


#undef SYNC
#ifdef SIMULATOR
#define SYNC
#else
#define USETHREADS
#endif

#define MAX_CHARS LCD_WIDTH/6
#define MAX_LINES LCD_HEIGHT/8
#define LINE_LENGTH 80

#define DIR_PREV  1
#define DIR_NEXT -1
#define DIR_NONE  0

#define PLUGIN_NEWSONG 10

/* Persistent configuration */
#define MIKMOD_CONFIGFILE             "mikmod.cfg"
#define MIKMOD_SETTINGS_MINVERSION    1
#define MIKMOD_SETTINGS_VERSION       1

#ifdef USETHREADS
#define EV_EXIT 9999
#define THREAD_STACK_SIZE DEFAULT_STACK_SIZE + 0x200
static unsigned int thread_id;
static struct event_queue thread_q SHAREDBSS_ATTR;
/* use long for aligning */
unsigned long thread_stack[THREAD_STACK_SIZE/sizeof(long)];
#endif

/* the current full file name */
static char np_file[MAX_PATH];
static int curfile = 0, direction = DIR_NEXT, entries = 0;

/* list of the mod files */
static char **file_pt;


/* The MP3 audio buffer which we will use as heap memory */
static unsigned char* audio_buffer;
/* amount of bytes left in audio_buffer */
static size_t audio_buffer_free;


bool quit;
int playingtime IBSS_ATTR;
MODULE *module IBSS_ATTR;
char gmbuf[BUF_SIZE*NBUF];


int textlines;
int vscroll = 0;
int hscroll = 0;
bool screenupdated = false;

enum {
    DISPLAY_INFO = 0,
    DISPLAY_SAMPLE,
    DISPLAY_INST,
    DISPLAY_COMMENTS,
} display;


/*
* strncat wrapper
*/
char* mmsupp_strncat(char *s1, const char *s2, size_t n)
{
    char *s = s1;
    /* Loop over the data in s1.  */
    while (*s != '\0')
        s++;
    /* s now points to s1's trailing null character, now copy
    up to n bytes from s2 into s1 stopping if a null character
    is encountered in s2.
    It is not safe to use strncpy here since it copies EXACTLY n
    characters, NULL padding if necessary.  */
    while (n != 0 && (*s = *s2++) != '\0')
    {
        n--;
        s++;
    }
    if (*s != '\0')
        *s = '\0';
    return s1;
}

/*
* sprintf wrapper
*/
int mmsupp_sprintf(char *buf, const char *fmt, ... )
{
    bool ok;
    va_list ap;

    va_start(ap, fmt);
    ok = rb->vsnprintf(buf, LINE_LENGTH, fmt, ap);
    va_end(ap);

    return ok;
}

/*
* printf wrapper
*/
void mmsupp_printf(const char *fmt, ...)
{
    static int p_xtpt = 0;
    char p_buf[LINE_LENGTH];
    /* bool ok; */
    va_list ap;

    va_start(ap, fmt);
    /* ok = */ (void)rb->vsnprintf(p_buf, sizeof(p_buf), fmt, ap);
    va_end(ap);

    int i=0;

    /* Device LCDs display newlines funny. */
    for(i=0; p_buf[i]!=0; i++)
        if(p_buf[i] == '\n')
            p_buf[i] = ' ';

    rb->lcd_putsxy(1, p_xtpt, (unsigned char *)p_buf);
    rb->lcd_update();

    p_xtpt += 8;
    if(p_xtpt > LCD_HEIGHT-8)
    {
        p_xtpt = 0;
        rb->lcd_clear_display();
    }
}


/************************* File Access ***************************/

/* support function for qsort() */
/* not used
static int compare(const void* p1, const void* p2)
{
    return rb->strcasecmp(*((char **)p1), *((char **)p2));
}
*/

static bool mod_ext(const char ext[])
{
    if(!ext)
        return false;
    if(!rb->strcasecmp(ext,".669") ||
       !rb->strcasecmp(ext,".amf") ||
       !rb->strcasecmp(ext,".asy") ||
       !rb->strcasecmp(ext,".dsm") ||
       !rb->strcasecmp(ext,".far") ||
       !rb->strcasecmp(ext,".gdm") ||
       !rb->strcasecmp(ext,".gt2") ||
       !rb->strcasecmp(ext,".imf") ||
       !rb->strcasecmp(ext,".it") ||
       !rb->strcasecmp(ext,".m15") ||
       !rb->strcasecmp(ext,".med") ||
       !rb->strcasecmp(ext,".mod") ||
       !rb->strcasecmp(ext,".mtm") ||
       !rb->strcasecmp(ext,".okt") ||
       !rb->strcasecmp(ext,".s3m") ||
       !rb->strcasecmp(ext,".stm") ||
       !rb->strcasecmp(ext,".stx") ||
       !rb->strcasecmp(ext,".ult") ||
       !rb->strcasecmp(ext,".uni") ||
       !rb->strcasecmp(ext,".xm") )
            return true;
    else
            return false;
}

/*Read directory contents for scrolling. */
static void get_mod_list(void)
{
    struct tree_context *tree = rb->tree_get_context();
    struct entry *dircache = rb->tree_get_entries(tree);
    int i;
    char *pname;

    file_pt = (char **) audio_buffer;

    /* Remove path and leave only the name.*/
    pname = rb->strrchr(np_file,'/');
    pname++;

    for (i = 0; i < tree->filesindir && audio_buffer_free > sizeof(char**); i++)
    {
        if (!(dircache[i].attr & ATTR_DIRECTORY)
            && mod_ext(rb->strrchr(dircache[i].name,'.')))
        {
            file_pt[entries] = dircache[i].name;
            /* Set Selected File. */
            if (!rb->strcmp(file_pt[entries], pname))
                curfile = entries;
            entries++;

            audio_buffer += (sizeof(char**));
            audio_buffer_free -= (sizeof(char**));
        }
    }
}

static int change_filename(int direct)
{
    bool file_erased = (file_pt[curfile] == NULL);
    direction = direct;

    curfile += (direct == DIR_PREV? entries - 1: 1);
    if (curfile >= entries)
        curfile -= entries;

    if (file_erased)
    {
        /* remove 'erased' file names from list. */
        int count, i;
        for (count = i = 0; i < entries; i++)
        {
            if (curfile == i)
                curfile = count;
            if (file_pt[i] != NULL)
                file_pt[count++] = file_pt[i];
        }
        entries = count;
    }

    if (entries == 0)
    {
        rb->splash(HZ, "No supported files");
        return PLUGIN_ERROR;
    }

    rb->strcpy(rb->strrchr(np_file, '/')+1, file_pt[curfile]);

    return PLUGIN_NEWSONG;
}

/*****************************************************************************
* Playback
*/

bool swap = false;
bool lastswap = true;

static inline void synthbuf(void)
{
    char *outptr;

#ifndef SYNC
    if (lastswap == swap) return;
    lastswap = swap;

    outptr = (swap ? gmbuf : gmbuf + BUF_SIZE);
#else
    outptr = gmbuf;
#endif

    VC_WriteBytes(outptr, BUF_SIZE);
}

static void get_more(const void** start, size_t* size)
{
#ifndef SYNC
    if (lastswap != swap)
    {
        //printf("Buffer miss!");
    }

#else
    synthbuf();
#endif

    *size = BUF_SIZE;
#ifndef SYNC
    *start = swap ? gmbuf : gmbuf + BUF_SIZE;
    swap = !swap;
#else
    *start = gmbuf;
#endif
}

static void showinfo(void)
{
    char statustext[LINE_LENGTH];

    if (!module)
    {
        return;
    }

    rb->lcd_clear_display();

    playingtime = (int)(module->sngtime >> 10);
    sprintf(statustext, "Name: %s", module->songname);
    rb->lcd_putsxy(1, 1, statustext);
    sprintf(statustext, "Type: %s", module->modtype);
    rb->lcd_putsxy(1, 11, statustext);
    
    sprintf(statustext, "Samples: %d", module->numsmp);
    rb->lcd_putsxy(1, 21, statustext);

    if ( module->flags & UF_INST )
    {
        sprintf(statustext, "Instruments: %d", module->numins);
        rb->lcd_putsxy(1, 31, statustext);
    }

    sprintf(statustext, "pat: %03d/%03d  %2.2X", 
        module->sngpos, module->numpos - 1, module->patpos);
    rb->lcd_putsxy(1, 51, statustext);

    sprintf(statustext, "spd: %d/%d", 
        module->sngspd, module->bpm);
    rb->lcd_putsxy(1, 61, statustext);

    sprintf(statustext, "vol: %ddB", rb->global_settings->volume);
    rb->lcd_putsxy(1, 71, statustext);

    sprintf(statustext, "time: %d:%02d", 
        (playingtime / 60) % 60, playingtime % 60);
    rb->lcd_putsxy(1, 81, statustext);

    if (module->flags & UF_NNA)
    {
        sprintf(statustext, "chn: %d/%d+%d->%d",
            module->realchn, module->numchn, 
            module->totalchn - module->realchn,
            module->totalchn);
    }
    else
    {
        sprintf(statustext, "chn: %d/%d",
            module->realchn, module->numchn);
    }
    rb->lcd_putsxy(0, 91, statustext);

    rb->lcd_update();
}

static void showsamples(void)
{
    int i;
    char statustext[LINE_LENGTH];

    if ( screenupdated )
    {
        return;
    }
    rb->lcd_clear_display();
    for( i=0; i<MAX_LINES && i+vscroll<module->numsmp; i++ )
    {
        sprintf(statustext, "%02d %s", i+vscroll+1, module->samples[i+vscroll].samplename);
        rb->lcd_putsxy(1, 1+(8*i), statustext);
    }
    rb->lcd_update();
    screenupdated = true;
}

static void showinstruments(void)
{
    int i;
    char statustext[LINE_LENGTH];

    if ( screenupdated )
    {
        return;
    }
    rb->lcd_clear_display();
    for( i=0; i<MAX_LINES && i+vscroll<module->numins; i++ )
    {
        sprintf(statustext, "%02d %s", i+vscroll+1, module->instruments[i+vscroll].insname);
        rb->lcd_putsxy(1, 1+(8*i), statustext);
    }
    rb->lcd_update();
    screenupdated = true;
}

static void showcomments(void)
{
    int i, j=0, k=0, l;
    char statustext[LINE_LENGTH];

    if ( screenupdated )
    {
        return;
    }
    rb->lcd_clear_display();

    for(i=0; module->comment[i]!='\0'; i++)
    {
        if(module->comment[i] != '\n')
        {
            statustext[j] = module->comment[i];
            j++;
        }

        if(module->comment[i] == '\n' || j>LINE_LENGTH-1)
        {
            rb->lcd_putsxy(1-(6*hscroll), 1+(8*k)-(8*vscroll), statustext);
            for( l=0; l<LINE_LENGTH; l++ )
            {
                statustext[l] = 0;
            }
            k++;
            j=0;
        }
    }
    if (j>0)
    {
        rb->lcd_putsxy(1-(6*hscroll), 1+(8*k)-(8*vscroll), statustext);
    }

    rb->lcd_update();
    screenupdated = true;
}

static void changedisplay(void)
{
    display = (display+1) % 4;

    if (display == DISPLAY_SAMPLE)
    {
        textlines = module->numsmp;
    }

    if (display == DISPLAY_INST)
    {
        if ( module->flags & UF_INST )
        {
            textlines = module->numins;
        }
        else
        {
            display = DISPLAY_COMMENTS;
        }
    }

    if (display == DISPLAY_COMMENTS)
    {
        if (module->comment)
        {
            textlines = 100;
        }
        else
        {
            display = DISPLAY_INFO;
        }
    }
    screenupdated = false;
    vscroll = 0;
    hscroll = 0;
}

struct mikmod_settings
{
    int pansep;
    int reverb;
    bool interp;
    bool reverse;
    bool surround;
    bool boost;
};

static struct mikmod_settings settings =
{
    128,
    0,
    0,
    0,
    1,
    1
};

static struct mikmod_settings old_settings;

static struct configdata config[] =
{
    { TYPE_INT, 0, 128, { .int_p = &settings.pansep }, "Panning Separation", NULL},
    { TYPE_INT, 0, 15, { .int_p = &settings.reverb }, "Reverberation", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.interp }, "Interpolation", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.reverse }, "Reverse Channels", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.surround }, "Surround", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.boost }, "CPU Boost", NULL},
};

static void applysettings(void)
{
    md_pansep = settings.pansep;
    md_reverb = settings.reverb;
    md_mode = DMODE_STEREO | DMODE_16BITS | DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
    if ( settings.interp )
    {
        md_mode |= DMODE_INTERP;
    }
    if ( settings.reverse )
    {
        md_mode |= DMODE_REVERSE;
    }
    if ( settings.surround )
    {
        md_mode |= DMODE_SURROUND;
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if ( Player_Active() )
    {
        rb->cpu_boost(settings.boost);
    }
#endif
}

/**
  Shows the settings menu
 */
static int settings_menu(void)
{
    int selection = 0;

    MENUITEM_STRINGLIST(settings_menu, "Mikmod Settings", NULL,
                        ID2P(LANG_PANNING_SEPARATION),
                        ID2P(LANG_REVERBERATION),
                        ID2P(LANG_INTERPOLATION),
                        ID2P(LANG_SWAP_CHANNELS),
                        ID2P(LANG_MIKMOD_SURROUND),
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                        ID2P(LANG_CPU_BOOST)
#endif
                        );

    do
    {
        selection=rb->do_menu(&settings_menu,&selection, NULL, false);
        switch(selection)
        {
        case 0:
            rb->set_int(rb->str(LANG_PANNING_SEPARATION), "", 1,
                        &(settings.pansep),
                        NULL, 8, 0, 128, NULL );
            applysettings();
            break;

        case 1:
            rb->set_int(rb->str(LANG_REVERBERATION), "", 1,
                        &(settings.reverb),
                        NULL, 1, 0, 15, NULL );
            applysettings();
            break;

        case 2:
            rb->set_bool(rb->str(LANG_INTERPOLATION), &(settings.interp));
            applysettings();
            break;

        case 3:
            rb->set_bool(rb->str(LANG_SWAP_CHANNELS), &(settings.reverse));
            applysettings();
            break;

        case 4:
            rb->set_bool(rb->str(LANG_MIKMOD_SURROUND), &(settings.surround));
            applysettings();
            break;

        case 5:
            rb->set_bool(rb->str(LANG_CPU_BOOST), &(settings.boost));
            applysettings();
            break;

        case MENU_ATTACHED_USB:
            return PLUGIN_USB_CONNECTED;
        }
    } while ( selection >= 0 );
    return 0;
}

/**
  Show the main menu
 */
static int main_menu(void)
{
    int selection = 0;
    int result;

    MENUITEM_STRINGLIST(main_menu,"Mikmod Main Menu",NULL,
                        ID2P(LANG_SETTINGS),
                        ID2P(LANG_RETURN),
                        ID2P(LANG_MENU_QUIT));
    while (1)
    {
        switch (rb->do_menu(&main_menu,&selection, NULL, false))
        {
        case 0:
            result = settings_menu();
            if ( result != 0 ) return result;
            break;

        case 1:
            return 0;

        case 2:
            return -1;

        case MENU_ATTACHED_USB:
            return PLUGIN_USB_CONNECTED;

        default:
            return 0;
        }
    }
}

#ifdef USETHREADS
/* double buffering thread */
static void thread(void)
{
    struct queue_event ev;

    while (1)
    {
        if (rb->queue_empty(&thread_q))
        {
            synthbuf();
            rb->yield();
        }
        else rb->queue_wait(&thread_q, &ev);
        switch (ev.id) {
            case EV_EXIT:
                return;
        }
    }
}
#endif

static void mm_errorhandler(void)
{
    rb->splashf(HZ, "%s", MikMod_strerror(MikMod_errno));
    quit = true;
}

static int playfile(char* filename)
{
    int vol = 0;
    int button;
    int retval = PLUGIN_OK;
    bool changingpos = false;
    int menureturn;

    playingtime = 0;

    rb->splashf(HZ, "Loading %s", filename);

    module = Player_Load(filename, 64, 0);

    if (!module)
    {
        rb->splashf(HZ, "%s", MikMod_strerror(MikMod_errno));
        retval = PLUGIN_ERROR;
        quit = true;
    }
    else
    {
        display = DISPLAY_INFO;
        Player_Start(module);
        rb->pcmbuf_fade(false, true);
        rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, get_more, NULL, 0);
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if ( settings.boost )
        rb->cpu_boost(true);
#endif
#ifdef USETHREADS
    rb->queue_init(&thread_q, true);
    if ((thread_id = rb->create_thread(thread, thread_stack,
        sizeof(thread_stack), 0, "render buffering thread" 
        IF_PRIO(, PRIORITY_PLAYBACK)
        IF_COP(, CPU))) == 0)
    {
        rb->splash(HZ, "Cannot create thread!");
        return PLUGIN_ERROR;
    }
#endif

    while (!quit && Player_Active() && retval == PLUGIN_OK)
    {
#if !defined(SYNC) && !defined(USETHREADS)
        synthbuf();
#endif
        switch (display)
        {
        case DISPLAY_SAMPLE:
            showsamples();
            break;
        case DISPLAY_INST:
            showinstruments();
            break;
        case DISPLAY_COMMENTS:
            showcomments();
            break;
        default:
            showinfo();
        }

        rb->yield();

        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        button = rb->get_action(CONTEXT_WPS, TIMEOUT_NOBLOCK);
        switch (button)
        {
        case ACTION_WPS_VOLUP:
            if ( display != DISPLAY_INFO )
            {
                if ( textlines-vscroll >= MAX_LINES )
                {
                    vscroll++;
                    screenupdated = false;
                }
                break;
            }
            vol = rb->global_settings->volume;
            if (vol < rb->sound_max(SOUND_VOLUME))
            {
                vol++;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;

        case ACTION_WPS_VOLDOWN:
            if ( display != DISPLAY_INFO )
            {
                if ( vscroll > 0 )
                {
                    vscroll--;
                    screenupdated = false;
                }
                break;
            }
            vol = rb->global_settings->volume;
            if (vol > rb->sound_min(SOUND_VOLUME))
            {
                vol--;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;

        case ACTION_WPS_SKIPPREV:
            if(entries>1 && !changingpos)
            {
                if ((int)(module->sngtime >> 10) > 2)
                {
                    Player_SetPosition(0);
                    module->sngtime = 0;
                }
                else {
                    retval = change_filename(DIR_PREV);
                }
            }
            else
            {
                changingpos = false;
            }
            break;
        case ACTION_WPS_SEEKBACK:
            if ( display != DISPLAY_INFO )
            {
                if ( hscroll > 0 )
                {
                    hscroll--;
                    screenupdated = false;
                }
                break;
            }
            Player_PrevPosition();
            changingpos = true;
            break;

        case ACTION_WPS_SKIPNEXT:
            if(entries>1 && !changingpos)
            {
                retval = change_filename(DIR_NEXT);
            }
            else
            {
                changingpos = false;
            }
            break;
        case ACTION_WPS_SEEKFWD:
            if ( display != DISPLAY_INFO )
            {
                hscroll++;
                screenupdated = false;
                break;
            }
            Player_NextPosition();
            changingpos = true;
            break;

        case ACTION_WPS_PLAY:
            rb->mixer_channel_play_pause(PCM_MIXER_CHAN_PLAYBACK, Player_Paused());
            Player_TogglePause();
            break;

        case ACTION_WPS_BROWSE:
            changedisplay();
            break;

        case ACTION_WPS_MENU:
            menureturn = main_menu();
            if ( menureturn != 0 )
            {
                quit = true;
                if ( menureturn == PLUGIN_USB_CONNECTED )
                {
                    retval = menureturn;
                }
            }
            rb->lcd_setfont(FONT_SYSFIXED);
            screenupdated = false;
            break;
            
        case ACTION_WPS_STOP:
            quit = true;
            break;
            
        default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            {
                quit = true;
                retval = PLUGIN_USB_CONNECTED;
            }
        }
    }

#ifdef USETHREADS
    rb->queue_post(&thread_q, EV_EXIT, 0);
    rb->thread_wait(thread_id);
    rb->queue_delete(&thread_q);
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if ( settings.boost )
        rb->cpu_boost(false);
#endif

    Player_Stop();
    Player_Free(module);
    
    memset(gmbuf, '\0', sizeof(gmbuf));
    
    if ( retval == PLUGIN_OK && entries > 1 && !quit )
    {
        retval = change_filename(DIR_NEXT);
    }
    
    return retval;
}

/*
* Plugin entry point
*
*/
enum plugin_status plugin_start(const void* parameter)
{
    enum plugin_status retval;
    int orig_samplerate = rb->mixer_get_frequency();

    if (parameter == NULL)
    {
        rb->splash(HZ*2, " Play .mod, .it, .s3m, .xm file ");
        return PLUGIN_OK;
    }

    rb->lcd_setfont(FONT_SYSFIXED);

    rb->talk_force_shutup();
    rb->pcm_play_stop();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->mixer_set_frequency(SAMPLE_RATE);

    audio_buffer = rb->plugin_get_audio_buffer((size_t *)&audio_buffer_free);
    
    rb->strcpy(np_file, parameter);
    get_mod_list();
    if(!entries) {
        return PLUGIN_ERROR;
    }

    //add_pool(audio_buffer, audio_buffer_free);
    init_memory_pool(audio_buffer_free, audio_buffer);
    
    MikMod_RegisterDriver(&drv_nos);
    MikMod_RegisterAllLoaders();
    MikMod_RegisterErrorHandler(mm_errorhandler);

    md_mixfreq = SAMPLE_RATE;

    configfile_load(MIKMOD_CONFIGFILE, config,
                    ARRAYLEN(config), MIKMOD_SETTINGS_MINVERSION);
    rb->memcpy(&old_settings, &settings, sizeof (settings));
    applysettings();

    if (MikMod_Init(""))
    {
        rb->splashf(HZ, "%s", MikMod_strerror(MikMod_errno));
        return PLUGIN_ERROR;
    }

    do
    {
        retval = playfile(np_file);
    } while (retval == PLUGIN_NEWSONG);

    MikMod_Exit();

    rb->pcmbuf_fade(false, false);
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
    rb->mixer_set_frequency(orig_samplerate);

    if (retval == PLUGIN_OK)
    {
        rb->splash(0, "Saving Settings");
        if (rb->memcmp(&settings, &old_settings, sizeof (settings)))
        {
            configfile_save(MIKMOD_CONFIGFILE, config,
                        ARRAYLEN(config), MIKMOD_SETTINGS_MINVERSION);
        }
    }
    
    destroy_memory_pool(audio_buffer);

    return retval;
}
