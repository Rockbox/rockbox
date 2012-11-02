struct gif_decoder {
    unsigned char *mem;
    size_t mem_size;
    int width;
    int height;
    size_t native_img_size;
    int error;
};

void gif_decoder_init(struct gif_decoder *decoder, void *mem, size_t size);
void gif_open(char *filename, struct gif_decoder *d);
void gif_decode(struct gif_decoder *d, void (*pf_progress)(int current, int total));

