#ifdef ROCKBOX
#include "lib/display_text.h"
#endif

/* defined in help/ */
extern const char help_text[];
#if defined(ROCKBOX) || defined(LZ4TINY)
extern const char quick_help_text[];
extern const unsigned short help_text_len, quick_help_text_len, help_text_words;
#endif

#if defined(ROCKBOX)
extern struct style_text help_text_style[];
#endif
