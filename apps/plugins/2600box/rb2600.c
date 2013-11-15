#include "rbconfig.h"

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

/**v2600 headers***********/
#include "types.h"      /**/
#include "display.h"    /**/
#include "keyboard.h"   /**/
#include "realjoy.h"    /**/
#include "files.h"      /**/
#include "config.h"     /**/
#include "vmachine.h"   /**/
#include "profile.h"    /**/
#include "options.h"    /**/
/*************************/
/* The mainloop from cpu.c */
extern void mainloop (void);

enum plugin_status plugin_start(const void* parameter)
{

#if CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif
    rb->splash(HZ, "Welcome to Atari 2600 :)");

#ifdef USE_IRAM
   /* We need to stop audio playback in order to use IRAM */
   rb->audio_stop();

   memcpy(iramstart, iramcopy, iramend-iramstart);
   memset(iedata, 0, iend - iedata);
#endif


#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

/* main loop here */
  if ( parameter != NULL )
	parse_options (parameter);
  else 
	parse_options ( "" );
      /* Initialise the 2600 hardware */
  init_hardware ();

  /* Turn the virtual TV on. */
  tv_on (NULL, NULL);

  /* Turn on sound */
  sound_init ();

  init_realjoy ();

  /* load cartridge image */
  if (loadCart ())
    {
      rb->splash (HZ, "Error loading cartridge image.\n");
    }

  /* Cannot be done until file is loaded */
  init_banking();
  mainloop();


#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

#if CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif

return PLUGIN_OK;
}
