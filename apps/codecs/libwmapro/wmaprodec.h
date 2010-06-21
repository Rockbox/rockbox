#include "avcodec.h"

av_cold int decode_end(AVCodecContext *avctx);
av_cold int decode_init(AVCodecContext *avctx);
int decode_packet(AVCodecContext *avctx,
                  void *data, int *data_size, AVPacket* avpkt);
