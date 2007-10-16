
#include "plugin.h"

enum mpeg_option_id
{
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    MPEG_OPTION_DITHERING,
#endif
    MPEG_OPTION_DISPLAY_FPS,
    MPEG_OPTION_LIMIT_FPS,
    MPEG_OPTION_SKIP_FRAMES,
};

enum mpeg_start_id
{
    MPEG_START_RESTART,
    MPEG_START_RESUME,
    MPEG_START_SEEK,
    MPEG_START_QUIT,
};

enum mpeg_menu_id
{
    MPEG_MENU_DISPLAY_SETTINGS,
    MPEG_MENU_CLEAR_RESUMES,
    MPEG_MENU_QUIT,
};

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
enum mpeg_start_id mpeg_start_menu(int play_time, int in_file);
enum mpeg_menu_id mpeg_menu(void);
void init_settings(const char* filename);
void save_settings(void);
void clear_resume_count(void);
