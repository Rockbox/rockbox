
#include "plugin.h"

struct mpeg_settings {
    int showfps;               /* flag to display fps */
    int limitfps;              /* flag to limit fps */
    int skipframes;            /* flag to skip frames */
    int resume_count;          /* total # of resumes in config file */
    int resume_time;           /* resume time for current mpeg (in half minutes) */
    char resume_filename[128]; /* filename of current mpeg */

#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    int displayoptions;
#endif
};

extern struct mpeg_settings settings;

int get_start_time(int play_time, int in_file);
int mpeg_start_menu(int play_time, int in_file);
bool mpeg_menu(void);
void init_settings(const char* filename);
void save_settings(void);
void clear_resume_count(void);
