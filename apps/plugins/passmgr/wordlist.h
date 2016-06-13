#include "plugin.h"

/* the word list is BIG! */
#if PLUGIN_BUFFER_SIZE > 0x8000
#define PASSMGR_DICEWARE
extern const char *word_list[];
extern const size_t word_list_len;
#endif
