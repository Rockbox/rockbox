
#include "plugin.h"

struct mpeg_settings {
    int showfps;
    int limitfps;
    int skipframes;
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    unsigned displayoptions;
#endif
};

extern struct mpeg_settings settings;

bool mpeg_menu(void);
void init_settings(void);
void save_settings(void);
