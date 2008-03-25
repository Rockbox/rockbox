#include <stdio.h>
#include <stdlib.h>
#include "gwps.h"

#define MIN(x,y) ((x) > (y) ? (y) : (x))

bool debug_wps = true;
int wps_verbose_level = 0;

int read_bmp_file(char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format)
{
    return 0;
}

int errno;

int read_line(int fd, char* buffer, int buffer_size)
{
    int count = 0;
    int num_read = 0;
    
    errno = 0;

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

    return errno ? -1 : num_read;
}

bool load_wps_backdrop(char* filename)
{
    return true;
}

static char pluginbuf[PLUGIN_BUFFER_SIZE];

static int dummy_func1(void)
{
    return 0;
}

static unsigned dummy_func2(void)
{
    return 0;
}

void* plugin_get_buffer(size_t *buffer_size)
{
    *buffer_size = PLUGIN_BUFFER_SIZE;
    return pluginbuf;
}

struct screen screens[NB_SCREENS] =
{
    {
        .screen_type=SCREEN_MAIN,
        .width=LCD_WIDTH,
        .height=LCD_HEIGHT,
        .depth=LCD_DEPTH,
        .is_color=true,
        .has_disk_led=false,
        .getxmargin=dummy_func1,
        .getymargin=dummy_func1,
        .get_foreground=dummy_func2,
        .get_background=dummy_func2,
    },
#ifdef HAVE_REMOTE_LCD
    {
        .screen_type=SCREEN_REMOTE,
        .width=LCD_REMOTE_WIDTH,
        .height=LCD_REMOTE_HEIGHT,
        .depth=LCD_REMOTE_DEPTH,
        .is_color=false,/* No color remotes yet */
        .getxmargin=dummy_func,
        .getymargin=dummy_func,
        .get_foreground=dummy_func,
        .get_background=dummy_func,
    }
#endif
};

#ifdef HAVE_LCD_BITMAP
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height)
{
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
#endif


int main(int argc, char **argv)
{
    int res;
    int fd;
    int filearg = 1;

    struct wps_data wps;

    if (argc < 2) {
        printf("Usage: checkwps [OPTIONS] filename.wps\n");
        printf("\nOPTIONS:\n");
        printf("\t-v\tverbose\n");
        printf("\t-vv\tmore verbose\n");
        printf("\t-vvv\tvery verbose\n");
        return 1;
    }

    if (argv[1][0] == '-') {
        filearg++;
        int i = 1;
        while (argv[1][i] && argv[1][i] == 'v') {
            i++;
            wps_verbose_level++;
        }
    }

    fd = open(argv[filearg], O_RDONLY);
    if (fd < 0) {
      printf("Failed to open %s\n",argv[1]);
      return 2;
    }
    close(fd);

    res = wps_data_load(&wps, &screens[0], argv[filearg], true);

    if (!res) {
      printf("WPS parsing failure\n");
      return 3;
    }

    printf("WPS parsed OK\n");
    return 0;
}

