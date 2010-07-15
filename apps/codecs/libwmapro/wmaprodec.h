#include "../libasf/asf.h"

int decode_init(asf_waveformatex_t *wfx);
int decode_packet(asf_waveformatex_t *wfx,
                  void *data, int *data_size, void* pktdata, int size);
