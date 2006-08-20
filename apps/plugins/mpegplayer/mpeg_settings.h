
#include "plugin.h"

struct mpeg_settings {
    int showfps;
    int limitfps;
    int skipframes;
};

extern struct mpeg_settings settings;

bool mpeg_menu(void);
void init_settings(void);
void save_settings(void);
