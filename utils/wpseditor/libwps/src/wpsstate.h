#ifndef STATES_H
#define STATES_H
//
struct trackstate
{
    char* title;
    char* artist;
    char* album;
    char* genre_string;
    char* disc_string;
    char* track_string;
    char* year_string;
    char* composer;
    char* comment;
    char* albumartist;
    char* grouping;
    int discnum;
    int tracknum;
    int version;
    int layer;
    int year;

    int length;
    int elapsed;
};

struct wpsstate{
    int volume;
    int fontheight;
    int fontwidth;
    int battery_level;
    int audio_status;
};
#endif
