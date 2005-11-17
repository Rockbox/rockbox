#ifndef _GWPS_COMMON_
#define _GWPS_COMMON_
#include <stdbool.h>
#include <sys/types.h> /* for size_t */

/* to avoid the unnecessary include if gwps.h */
struct mp3entry;
struct gui_img;
struct wps_data;
struct gui_wps;
struct align_pos;

void gui_wps_format_time(char* buf, int buf_size, long time);
void fade(bool fade_in);
void gui_wps_format(struct wps_data *data, const char *bmpdir, size_t bmpdirlen);
bool gui_wps_refresh(struct gui_wps *gwps, int ffwd_offset,
                     unsigned char refresh_mode);
bool gui_wps_display(void);
bool setvol(void);
bool update(struct gui_wps *gwps);
bool ffwd_rew(int button);
#ifdef WPS_KEYLOCK
void display_keylock_text(bool locked);
void waitfor_nokey(void);
#endif
#endif

