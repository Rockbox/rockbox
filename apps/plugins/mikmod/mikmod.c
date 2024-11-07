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
#define MIKMOD_SETTINGS_VERSION       2

#ifdef USETHREADS
#define EV_EXIT 9999
#define THREAD_STACK_SIZE DEFAULT_STACK_SIZE + 0x200
static unsigned int thread_id;
static struct event_queue thread_q SHAREDBSS_ATTR;
/* use long for aligning */
unsigned long thread_stack[THREAD_STACK_SIZE/sizeof(long)];
#endif

static int font_h, font_w;

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
       !rb->strcasecmp(ext,".umx") ||
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
        rb->splash(HZ, ID2P(LANG_NO_FILES));
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
    rb->lcd_putsxy(1, 1 + 1 * font_h, statustext);

    sprintf(statustext, "Samples: %d", module->numsmp);
    rb->lcd_putsxy(1, 1 + 2 * font_h, statustext);

    if ( module->flags & UF_INST )
    {
        sprintf(statustext, "Instruments: %d", module->numins);
        rb->lcd_putsxy(1, 1 + 3 * font_h, statustext);
    }

    sprintf(statustext, "pat: %03d/%03d  %2.2X",
        module->sngpos, module->numpos - 1, module->patpos);
    rb->lcd_putsxy(1, 1 + 5 * font_h, statustext);

    sprintf(statustext, "spd: %d/%d",
        module->sngspd, module->bpm);
    rb->lcd_putsxy(1, 1 + 6 * font_h, statustext);

    sprintf(statustext, "vol: %ddB", rb->global_settings->volume);
    rb->lcd_putsxy(1, 1 + 7 * font_h, statustext);

    sprintf(statustext, "time: %d:%02d",
        (playingtime / 60) % 60, playingtime % 60);
    rb->lcd_putsxy(1, 1 + 8 * font_h, statustext);

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
    rb->lcd_putsxy(0, 1 + 9 * font_h, statustext);

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
        rb->lcd_putsxy(1, 1+(font_h*i), statustext);
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
        sprintf(statustext, "%02d %s", i+vscroll+1, module->instruments[i+vscroll].insname ? module->instruments[i+vscroll].insname : "[n/a]");
        rb->lcd_putsxy(1, 1+(font_h*i), statustext);
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
            rb->lcd_putsxy(1-(font_w*hscroll), 1+(font_h*k)-(font_h*vscroll), statustext);
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
        rb->lcd_putsxy(1-(font_w*hscroll), 1+(font_h*k)-(font_h*vscroll), statustext);
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
    int sample_rate;
    bool interp;
    bool reverse;
    bool surround;
    bool hqmixer;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    bool boost;
#endif
};

static struct mikmod_settings settings =
{
    .pansep = 128,
    .reverb = 0,
    .sample_rate = -1,
    .interp = 0,
    .reverse = 0,
    .surround = 1,
    .hqmixer = 0,
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    .boost = 1,
#endif
};

static struct mikmod_settings old_settings;

static const struct configdata config[] =
{
    { TYPE_INT, 0, 128, { .int_p = &settings.pansep }, "Panning Separation", NULL},
    { TYPE_INT, 0, 15, { .int_p = &settings.reverb }, "Reverberation", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.interp }, "Interpolation", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.reverse }, "Reverse Channels", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.surround }, "Surround", NULL},
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.hqmixer }, "HQ Mixer", NULL},
    { TYPE_INT, 0, HW_NUM_FREQ-1, { .int_p = &settings.sample_rate }, "Sample Rate", NULL},
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    { TYPE_BOOL, 0, 1, { .bool_p = &settings.boost }, "CPU Boost", NULL},
#endif
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
#ifndef NO_HQMIXER
    if ( settings.hqmixer )
    {
        md_mode |= DMODE_HQMIXER;
    }
#endif

    if (md_mixfreq != rb->hw_freq_sampr[settings.sample_rate]) {
        md_mixfreq = rb->hw_freq_sampr[settings.sample_rate];
//	MikMod_Reset("");  BROKEN!
	rb->pcm_play_stop();
	rb->mixer_set_frequency(md_mixfreq);
	rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, get_more, NULL, 0);
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if ( Player_Active() )
    {
        rb->cpu_boost(settings.boost);
    }
#endif
}

static const struct opt_items sr_names[HW_NUM_FREQ] = {
        HW_HAVE_192_([HW_FREQ_192] = { "192kHz",     TALK_ID(192, UNIT_KHZ) },)
        HW_HAVE_176_([HW_FREQ_176] = { "176.4kHz",   TALK_ID(176, UNIT_KHZ) },)
        HW_HAVE_96_([HW_FREQ_96] = { "96kHz",     TALK_ID(96, UNIT_KHZ) },)
        HW_HAVE_88_([HW_FREQ_88] = { "88.2kHz",   TALK_ID(88, UNIT_KHZ) },)
        HW_HAVE_64_([HW_FREQ_64] = { "64kHz",     TALK_ID(64, UNIT_KHZ) },)
        HW_HAVE_48_([HW_FREQ_48] = { "48kHz",     TALK_ID(48, UNIT_KHZ) },)
        HW_HAVE_44_([HW_FREQ_44] = { "44.1kHz",   TALK_ID(44, UNIT_KHZ) },)
        HW_HAVE_32_([HW_FREQ_32] = { "32kHz",     TALK_ID(32, UNIT_KHZ) },)
        HW_HAVE_24_([HW_FREQ_24] = { "24kHz",     TALK_ID(24, UNIT_KHZ) },)
        HW_HAVE_22_([HW_FREQ_22] = { "22.05kHz",  TALK_ID(22, UNIT_KHZ) },)
        HW_HAVE_16_([HW_FREQ_16] = { "16kHz",     TALK_ID(16, UNIT_KHZ) },)
        HW_HAVE_12_([HW_FREQ_12] = { "12kHz",     TALK_ID(12, UNIT_KHZ) },)
        HW_HAVE_11_([HW_FREQ_11] = { "11.025kHz", TALK_ID(11, UNIT_KHZ) },)
        HW_HAVE_8_( [HW_FREQ_8 ] = { "8kHz",      TALK_ID( 8, UNIT_KHZ) },)
};

/**
  Shows the settings menu
 */
static int settings_menu(void)
{
    int selection = 0;

    MENUITEM_STRINGLIST(settings_menu, ID2P(LANG_MIKMOD_SETTINGS), NULL,
                        ID2P(LANG_PANNING_SEPARATION),
                        ID2P(LANG_REVERBERATION),
                        ID2P(LANG_INTERPOLATION),
                        ID2P(LANG_SWAP_CHANNELS),
                        ID2P(LANG_MIKMOD_SURROUND),
                        ID2P(LANG_MIKMOD_HQMIXER),
                        ID2P(LANG_MIKMOD_SAMPLERATE),
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
            rb->set_int("Panning Separation", "", 1,
                        &(settings.pansep),
                        NULL, 8, 0, 128, NULL );
            applysettings();
            break;

        case 1:
            rb->set_int("Reverberation", "", 1,
                        &(settings.reverb),
                        NULL, 1, 0, 15, NULL );
            applysettings();
            break;

        case 2:
            rb->set_bool("Interpolation", &(settings.interp));
            applysettings();
            break;

        case 3:
            rb->set_bool("Reverse Channels", &(settings.reverse));
            applysettings();
            break;

        case 4:
            rb->set_bool("Surround", &(settings.surround));
            applysettings();
            break;

        case 5:
            rb->set_bool("HQ Mixer", &(settings.hqmixer));
            applysettings();
            break;

        case 6:
            rb->set_option("Sample Rate", &(settings.sample_rate), RB_INT, sr_names,
                           HW_NUM_FREQ, NULL);
            applysettings();
            break;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        case 7:
            rb->set_bool("CPU Boost", &(settings.boost));
            applysettings();
            break;
#endif

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

    MENUITEM_STRINGLIST(main_menu,ID2P(LANG_MIKMOD_MENU), NULL,
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
    struct queue_event ev = {
         .id = 0,
    };

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
    if (rb->global_settings->talk_menu) {
        rb->talk_id(LANG_ERROR_FORMATSTR, true);
        rb->talk_value_decimal(MikMod_errno, UNIT_INT, 0, true);
        rb->talk_force_enqueue_next();
    }
    rb->splashf(HZ, rb->str(LANG_ERROR_FORMATSTR), MikMod_strerror(MikMod_errno));
    quit = true;
}

static int playfile(char* filename)
{
    int button;
    int retval = PLUGIN_OK;
    bool changingpos = false;
    int menureturn;

    playingtime = 0;

    if (rb->global_settings->talk_menu) {
        rb->talk_id(LANG_WAIT, true);
        rb->talk_file_or_spell(NULL, filename, NULL, true);
        rb->talk_force_enqueue_next();
    }
    rb->splashf(HZ, "%s %s", rb->str(LANG_WAIT), filename);

    module = Player_Load(filename, 64, 0);

    if (!module)
    {
        mm_errorhandler();
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
    rb->cpu_boost(settings.boost);
#endif
#ifdef USETHREADS
    rb->queue_init(&thread_q, true);
    if ((thread_id = rb->create_thread(thread, thread_stack,
        sizeof(thread_stack), 0, "render buffering thread"
        IF_PRIO(, PRIORITY_PLAYBACK)
        IF_COP(, CPU))) == 0)
    {
        rb->splashf(HZ, ID2P(LANG_ERROR_FORMATSTR), "Cannot create thread!");
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

            rb->adjust_volume(1);
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

            rb->adjust_volume(-1);
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
            rb->lcd_setfont(FONT_UI);
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
        rb->splash(HZ*2, ID2P(LANG_NO_FILES));
        return PLUGIN_OK;
    }

    rb->lcd_getstringsize("A", NULL, &font_h);

    rb->talk_force_shutup();
    rb->pcm_play_stop();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

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

    configfile_load(MIKMOD_CONFIGFILE, config,
                    ARRAYLEN(config), MIKMOD_SETTINGS_MINVERSION);
    rb->memcpy(&old_settings, &settings, sizeof (settings));

    /* If there's no configured rate, use the default */
    if (settings.sample_rate == -1) {
	    int i;
	    for (i = 0 ; i < HW_NUM_FREQ ; i++) {
		    if (rb->hw_freq_sampr[i] == SAMPLE_RATE) {
			    settings.sample_rate = i;
			    break;
		    }
	    }
	    if (settings.sample_rate == -1) {
		    settings.sample_rate = HW_NUM_FREQ -1;
	    }
    }

    applysettings();

    if (MikMod_Init(""))
    {
        mm_errorhandler();
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
        if (rb->memcmp(&settings, &old_settings, sizeof (settings)))
        {
	    configfile_save(MIKMOD_CONFIGFILE, config,
                            ARRAYLEN(config), MIKMOD_SETTINGS_MINVERSION);
        }
    }

    destroy_memory_pool(audio_buffer);

    return retval;
}
