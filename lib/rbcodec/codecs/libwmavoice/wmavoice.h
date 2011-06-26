#include "avcodec.h"

#define ERROR_WMAPRO_IN_WMAVOICE -0x162

av_cold int wmavoice_decode_init(AVCodecContext *ctx);
int wmavoice_decode_packet(AVCodecContext *ctx, void *data,
                                  int *data_size, AVPacket *avpkt);
