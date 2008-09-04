#include <stdio.h>
#include <stdlib.h>
#include "dummies.h"
#include "proxy.h"
#include "api.h"
#include "gwps.h"
#include "gwps-common.h"
#include <string.h>

struct screen screens[NB_SCREENS];
struct wps_data wpsdata;
struct gui_wps gwps;
struct mp3entry id3;
struct mp3entry nid3;

extern void test_api(struct proxy_api *api);

bool debug_wps = true;
int wps_verbose_level = 0;
int errno_;
pfdebugf dbgf = 0;

static char pluginbuf[PLUGIN_BUFFER_SIZE];

const char* get_model_name(){
#ifdef MODEL_NAME
    return MODEL_NAME;
#else
    return "unknown";
#endif
}

int read_line(int fd, char* buffer, int buffer_size)
{
    int count = 0;
    int num_read = 0;

    errno_ = 0;

    while (count < buffer_size)
    {
        unsigned char c;

        if (1 != read(fd, &c, 1))
            break;

        num_read++;

        if ( c == '\n' )
            break;

        if ( c == '\r' )
            continue;

        buffer[count++] = c;
    }

    buffer[MIN(count, buffer_size - 1)] = 0;

    return errno_ ? -1 : num_read;
}

void* plugin_get_buffer(size_t *buffer_size)
{
    *buffer_size = PLUGIN_BUFFER_SIZE;
    return pluginbuf;
}

int checkwps(const char *filename, int verbose){
    int res;
    int fd;

    struct wps_data wps;
    wps_verbose_level = verbose;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
      DEBUGF1("Failed to open %s\n",filename);
      return 2;
    }
    close(fd);

    res = wps_data_load(&wps, &screens[0], filename, true);

    if (!res) {
      DEBUGF1("WPS parsing failure\n");
      return 3;
    }

    DEBUGF1("WPS parsed OK\n");
    return 0;
}

int wps_init(const char* filename,struct proxy_api *api, bool isfile){
    int res;
    if (!api)
        return 4;
    dummies_init();
    test_api(api);
    set_api(api);
    wps_data_init(&wpsdata);
    wps_verbose_level = api->verbose;
    res = wps_data_load(&wpsdata, &screens[0], filename, isfile);
    if (!res)
    {
      DEBUGF1("ERR: WPS parsing failure\n");
    } else
        DEBUGF1("WPS parsed OK\n");
    DEBUGF1("\n-------------------------------------------------\n");
    wps_state.paused = true;
    gwps.data = &wpsdata;
    gwps.display = &screens[0];
    gwps.state = &wps_state;
    gwps.state->id3 = &id3;
    gwps.state->nid3 = &nid3;
    gui_wps[0] = gwps;
    return (res?res:3);
}

int wps_display(){
    DEBUGF3("wps_display(): begin\n");
    int res = gui_wps_display();
    DEBUGF3("\nWPS %sdisplayed\n", (res ? "" : "not "));
    return res;
}
int wps_refresh(){
    DEBUGF3("-----------------<wps_refresh(): begin>-----------------\n");
    int res = gui_wps_refresh(&gwps, 0, WPS_REFRESH_ALL);
    DEBUGF3("\nWPS %srefreshed\n", (res ? "" : "not "));
    return res;
}
