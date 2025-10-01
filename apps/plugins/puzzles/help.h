#include <stdbool.h>

#ifdef ROCKBOX
#include "lib/display_text.h"
#endif

/* Normally, these are defined in help/*.c. If the game lacks help
 * text (i.e. it is an unfinished game), there are weak dummy
 * definitions in dummy/nullhelp.c. */
extern const char help_text[];
#if defined(ROCKBOX) || defined(LZ4TINY)
extern const char quick_help_text[];
extern const unsigned short help_text_len, quick_help_text_len, help_text_words;
#endif

#if defined(ROCKBOX)
extern struct style_text help_text_style[];
#endif

extern const bool help_text_valid, quick_help_valid;
