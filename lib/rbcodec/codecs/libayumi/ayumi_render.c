#include "ayumi_render.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ayumi.h"
#include "lzh.h"
#include "codeclib.h"

ayumi_render_t ay;

/* default panning settings, 7 stereo types */
static const double default_pan[7][3] = {
/*  A,    B,    C  */

  {0.50, 0.50, 0.50},		/* MONO */
  {0.10, 0.50, 0.90},		/* ABC */
  {0.10, 0.90, 0.50},		/* ACB */
  {0.50, 0.10, 0.90},		/* BAC */
  {0.90, 0.10, 0.50},		/* BCA */
  {0.50, 0.90, 0.10},		/* CAB */
  {0.90, 0.50, 0.10}		/* CBA */
};

static const char *chiptype_name[3] = {
  "AY-3-8910",
  "YM2149",
  "Unknown"
};

static const char *layout_name[9] = {
  "Mono",
  "ABC Stereo",
  "ACB Stereo",
  "BAC Stereo",
  "BCA Stereo",
  "CAB Stereo",
  "CBA Stereo",
  "Custom",
  "Unknown"
};

/* reader */

#define VTX_STRING_MAX 254

typedef struct {
  uchar *ptr;
  uint size;
} reader_t;

reader_t reader;

void Reader_Init(void *pBlock) {
  reader.ptr = (uchar *) pBlock;
  reader.size = 0;
}

uint Reader_ReadByte(void) {
  uint res;
  res = *reader.ptr++;
  reader.size += 1;
  return res;
}

uint Reader_ReadWord(void) {
  uint res;
  res = *reader.ptr++;
  res += *reader.ptr++ << 8;
  reader.size += 2;
  return res;
}

uint Reader_ReadDWord(void) {
  uint res;
  res = *reader.ptr++;
  res += *reader.ptr++ << 8;
  res += *reader.ptr++ << 16;
  res += *reader.ptr++ << 24;
  reader.size += 4;
  return res;
}

char *Reader_ReadString(void) {
  char *res;
  if (reader.ptr == NULL)
    return NULL;
  int len = strlen((const char *)reader.ptr);
  if (len > VTX_STRING_MAX)
    return NULL;
  res = reader.ptr;
  reader.ptr += len + 1;
  reader.size += len + 1;
  return res;
}

uchar *Reader_GetPtr(void) {
  return reader.ptr;
}

uint Reader_GetSize(void) {
  return reader.size;
}

/* ayumi_render */

static int AyumiRender_LoadInfo(void *pBlock, uint size)
{
  if (size < 20)
    return 0;

  Reader_Init(pBlock);

  uint hdr = Reader_ReadWord();

  if (hdr == 0x7961)
    ay.info.chiptype = VTX_CHIP_AY;
  else if (hdr == 0x6d79)
    ay.info.chiptype = VTX_CHIP_YM;
  else {
    return 0;
  }

  ay.info.layout = (vtx_layout_t)
    Reader_ReadByte();
  ay.info.loop = Reader_ReadWord();
  ay.info.chipfreq = Reader_ReadDWord();
  ay.info.playerfreq = Reader_ReadByte();
  ay.info.year = Reader_ReadWord();
  ay.data.regdata_size = Reader_ReadDWord();
  ay.info.frames = ay.data.regdata_size / 14;
  ay.info.title = Reader_ReadString();
  ay.info.author = Reader_ReadString();
  ay.info.from = Reader_ReadString();
  ay.info.tracker = Reader_ReadString();
  ay.info.comment = Reader_ReadString();

  ay.data.lzhdata_size = size - Reader_GetSize();
  ay.data.lzhdata = (uchar *)codec_malloc(ay.data.lzhdata_size);
  memcpy(ay.data.lzhdata, Reader_GetPtr(), ay.data.lzhdata_size);

  return 1;
}

int AyumiRender_LoadFile(void *pBlock, uint size)
{
  if (!AyumiRender_LoadInfo(pBlock, size))
    return 0;

  ay.data.regdata = (uchar *)codec_malloc(ay.data.regdata_size);
  if (ay.data.regdata == NULL)
    return 0;

  int bRet = LzUnpack(ay.data.lzhdata, ay.data.lzhdata_size,
                      ay.data.regdata, ay.data.regdata_size);

  if (bRet)
    return 0;

  return 1;
}

const char *AyumiRender_GetChipTypeName(vtx_chiptype_t chiptype)
{
  if (chiptype > VTX_CHIP_YM)
    chiptype = (vtx_chiptype_t) (VTX_CHIP_YM + 1);
  return chiptype_name[chiptype];
}

const char *AyumiRender_GetLayoutName(vtx_layout_t layout)
{
  if (layout > VTX_LAYOUT_CUSTOM)
    layout = (vtx_layout_t) (VTX_LAYOUT_CUSTOM + 1);
  return layout_name[layout];
}

int AyumiRender_AyInit(vtx_chiptype_t chiptype, uint samplerate,
			  uint chipfreq, double playerfreq, uint dcfilter)
{
  if (chiptype > VTX_CHIP_YM)
    return 0;
  if ((samplerate < 8000) || (samplerate > 768000))
    return 0;
  if ((chipfreq < 1000000) || (chipfreq > 2000000))
    return 0;
  if ((playerfreq < 1) || (playerfreq > 100))
    return 0;

  ay.is_ym = (chiptype == VTX_CHIP_YM) ? 1 : 0;
  ay.clock_rate = chipfreq;
  ay.sr = samplerate;

  ay.dc_filter_on = dcfilter ? 1 : 0;

  ay.frame = 0;
  ay.isr_counter = 1;
  ay.isr_step = playerfreq / samplerate;

  if (!ayumi_configure(&ay.ay, ay.is_ym, ay.clock_rate, ay.sr))
    return 0;

  return 1;
}

int AyumiRender_SetLayout(vtx_layout_t layout, uint eqpower)
{
  if (layout > VTX_LAYOUT_CUSTOM)
    return 0;
  ay.is_eqp = eqpower ? 1 : 0;

  switch (layout) {
  case VTX_LAYOUT_MONO:
  case VTX_LAYOUT_ABC:
  case VTX_LAYOUT_ACB:
  case VTX_LAYOUT_BAC:
  case VTX_LAYOUT_BCA:
  case VTX_LAYOUT_CAB:
  case VTX_LAYOUT_CBA:
    for (int i = 0; i < 3; i++)
      ay.pan[i] = default_pan[layout][i];
    break;
  case VTX_LAYOUT_CUSTOM:
    for (int i = 0; i < 3; i++)
      ay.pan[i] = 0; // no custom layout
    break;
  default:
    return 0;
  }

  for (int i = 0; i < 3; i++)
    ayumi_set_pan(&ay.ay, i, ay.pan[i], ay.is_eqp);

  return 1;
}

uint AyumiRender_GetPos(void)
{
  return ay.frame;
}

uint AyumiRender_GetMaxPos(void)
{
  return ay.info.frames;
}

static void AyumiRender_UpdateAyumiState(void)
{
  int r[16];

  if (ay.frame < ay.info.frames) {
    uchar *ptr = ay.data.regdata + ay.frame;
    for (int n = 0; n < 14; n++) {
      r[n] = *ptr;
      ptr += ay.info.frames;
    }
  } else {
    for (int n = 0; n < 14; n++) {
      r[n] = 0;
    }
  }

  ayumi_set_tone(&ay.ay, 0, (r[1] << 8) | r[0]);
  ayumi_set_tone(&ay.ay, 1, (r[3] << 8) | r[2]);
  ayumi_set_tone(&ay.ay, 2, (r[5] << 8) | r[4]);
  ayumi_set_noise(&ay.ay, r[6]);
  ayumi_set_mixer(&ay.ay, 0, r[7] & 1, (r[7] >> 3) & 1, r[8] >> 4);
  ayumi_set_mixer(&ay.ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, r[9] >> 4);
  ayumi_set_mixer(&ay.ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, r[10] >> 4);
  ayumi_set_volume(&ay.ay, 0, r[8] & 0xf);
  ayumi_set_volume(&ay.ay, 1, r[9] & 0xf);
  ayumi_set_volume(&ay.ay, 2, r[10] & 0xf);
  ayumi_set_envelope(&ay.ay, (r[12] << 8) | r[11]);
  if (r[13] != 255) {
    ayumi_set_envelope_shape(&ay.ay, r[13]);
  }
}

int AyumiRender_Seek(ulong nSample)
{
  ulong samples = 0;

  ay.frame = 0;
  ay.isr_counter = 1;

  ayumi_configure(&ay.ay, ay.is_ym, ay.clock_rate, ay.sr);

  for (int i = 0; i < 3; i++)
    ayumi_set_pan(&ay.ay, i, ay.pan[i], ay.is_eqp);

  while (samples < nSample) {
    ay.isr_counter += ay.isr_step;
    if (ay.isr_counter >= 1) {
      ay.isr_counter -= 1;
      AyumiRender_UpdateAyumiState();
      ay.frame += 1;
    }
    ayumi_seek(&ay.ay);
    samples++;
  }

  return 1;
}

ulong AyumiRender_AySynth(void *pBuffer, ulong nSamples)
{
  ulong samples = 0;
  short *out = (int16_t *) pBuffer;

  for (ulong i = 0; i < nSamples; i++) {
    ay.isr_counter += ay.isr_step;
    if (ay.isr_counter >= 1) {
      ay.isr_counter -= 1;
      AyumiRender_UpdateAyumiState();
      ay.frame += 1;
    }
    ayumi_process(&ay.ay);
    if (ay.dc_filter_on) {
      ayumi_remove_dc(&ay.ay);
    }
    out[0] = (int16_t)(ay.ay.left * 16383);
    out[1] = (int16_t)(ay.ay.right * 16383);
    out += 2;
    samples++;
  }

  return samples;
}
