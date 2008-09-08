#define TDSPEED_OUTBUFSIZE 4096

bool tdspeed_init(int samplerate, bool stereo, int factor);
int tdspeed_apply(int32_t *buf_out[2], int32_t *buf_in[2], int data_len,
                  int last, int out_size);

long tdspeed_est_output_size(long size);
long tdspeed_est_input_size(long size);
int tdspeed_doit(int32_t *src[], int count);
