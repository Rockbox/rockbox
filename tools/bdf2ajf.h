/**
* Copyright (C) 2002 Alex Gitelman
*
*/
#ifndef __BDF2AJF__
#define __BDF2AJF__

#include "../firmware/ajf.h"


#define STARTFONT "STARTFONT"
#define ENDFONT "ENDFONT"
#define COMMENT  "COMMENT"
#define FONT "FONT"
#define SIZE "SIZE"
#define FONTBOUNDINGBOX  "FONTBOUNDINGBOX"
#define STARTPROPERTIES   "STARTPROPERTIES"
#define ENDPROPERTIES   "ENDPROPERTIES"
#define CHARS   "CHARS"
#define STARTCHAR   "STARTCHAR"
#define ENDCHAR   "ENDCHAR"
#define ENCODING   "ENCODING"
#define SWIDTH   "SWIDTH"
#define DWIDTH   "DWIDTH"
#define BBX   "BBX"
#define BITMAP   "BITMAP"

typedef struct 
{
    char *glyph_name;
    int encoding;
    int swidth_x;
    int swidth_y;
    int dwidth_x;
    int dwidth_y;
    int bbx_width;
    int bbx_height;
    int bbx_disp_x;
    int bbx_disp_y;
    unsigned char *bitmap;
    short bitmap_len;
} BDF_GLYPH;

typedef struct 
{
    char *bdf_ver;
    char *name;
    int point_size;
    int x_res;
    int y_res;
    int bound_width;
    int bound_height;
    int bound_disp_x;
    int bound_disp_y;
    int prop_count;
    char **prop_name;
    char **prop_value;
    int char_count;
    BDF_GLYPH** glyph;
    BDF_GLYPH* enc_table[256];
} BDF;

BDF* readFont(const char *name);
BDF_GLYPH* getGlyph(unsigned char c, BDF* bdf, short* enc_map);
void getBitmap(BDF_GLYPH* g, unsigned char* src);

void test_print(unsigned char c, BDF* font, short *map);
void test_print2(unsigned char *src, int height, int len);


extern short win_koi_map[];

extern int _font_error_code;
extern char _font_error_msg[];
void report_error(int code, const char *msg);
void writeAJF(BDF* bdf, const char* fname);






#endif

