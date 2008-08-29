#ifndef PROXY_H
#define PROXY_h

#include "screen_access.h"
#include "api.h"
#include "defs.h"

#define DEBUGF  dbgf
#define DEBUGF1 dbgf
#define DEBUGF2(...)
#define DEBUGF3(...)

EXPORT int checkwps(const char *filename, int verbose);
EXPORT int wps_init(const char* filename,struct proxy_api *api,bool isfile);
EXPORT int wps_display();
EXPORT int wps_refresh();

const char* get_model_name();

extern struct screen screens[NB_SCREENS];
extern bool   debug_wps;
extern int    wps_verbose_level;


#endif
