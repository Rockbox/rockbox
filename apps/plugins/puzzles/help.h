#ifdef ROCKBOX
#include "lib/display_text.h"
#endif

/* defined in help/ */
extern const char help_text[];
#ifdef ROCKBOX
extern const char quick_help_text[];
extern struct style_text help_text_style[];
extern const unsigned short help_text_len, quick_help_text_len, help_text_words;
#endif
