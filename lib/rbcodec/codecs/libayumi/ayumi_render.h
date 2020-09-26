#ifndef AYUMI_RENDER_H
#define AYUMI_RENDER_H

#include "ayumi.h"

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

typedef enum {
  VTX_CHIP_AY = 0,		/* emulate AY */
  VTX_CHIP_YM			/* emulate YM */
} vtx_chiptype_t;

typedef enum {
  VTX_LAYOUT_MONO = 0,
  VTX_LAYOUT_ABC,
  VTX_LAYOUT_ACB,
  VTX_LAYOUT_BAC,
  VTX_LAYOUT_BCA,
  VTX_LAYOUT_CAB,
  VTX_LAYOUT_CBA,
  VTX_LAYOUT_CUSTOM
} vtx_layout_t;

typedef struct {
  vtx_chiptype_t chiptype;	/* Type of sound chip */
  vtx_layout_t layout;		/* stereo layout */
  uint loop;			/* song loop */
  uint chipfreq;		/* AY chip freq (1773400 for ZX) */
  uint playerfreq;		/* 50 Hz for ZX, 60 Hz for yamaha */
  uint year;			/* year song composed */
  char *title;			/* song title */
  char *author;			/* song author */
  char *from;			/* song from */
  char *tracker;		/* tracker */
  char *comment;		/* comment */
  uint frames;			/* number of AY data frames */
} vtx_info_t;

typedef struct {
  uchar *lzhdata;		/* packed song data */
  uint lzhdata_size;		/* size of packed data */
  uchar *regdata;		/* unpacked song data */
  uint regdata_size;		/* size of unpacked data */
} vtx_data_t;

typedef struct {
  uint frame;			/* current frame position */
  double isr_step;
  double isr_counter;

  int dc_filter_on;

  int is_ym;
  double clock_rate;
  int sr;

  double pan[3];
  int is_eqp;

  struct ayumi ay;		/* ayumi structure */
  vtx_data_t data;		/* packed & unpacked vtx data */
  vtx_info_t info;		/* vtx info */
} ayumi_render_t;

int AyumiRender_LoadFile(void *pBlock, uint size);

const char *AyumiRender_GetChipTypeName(vtx_chiptype_t chiptype);
const char *AyumiRender_GetLayoutName(vtx_layout_t layout);

uint AyumiRender_GetPos(void);
uint AyumiRender_GetMaxPos(void);

int AyumiRender_AyInit(vtx_chiptype_t chiptype, uint samplerate, uint chipfreq,
           double playerfreq, uint dcfilter);
int AyumiRender_SetLayout(vtx_layout_t layout, uint eqpower);

int AyumiRender_Seek(ulong nSample);

ulong AyumiRender_AySynth(void *pBuffer, ulong nSamples);

#endif /* ifndef AYUMI_RENDER_H */
