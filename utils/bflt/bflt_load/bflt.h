int read_header(int fd, struct flat_hdr * header);
int check_header(struct flat_hdr * header);
int copy_segments(int fd, struct flat_hdr * header, void * dram, ssize_t dram_size, void * iram, ssize_t iram_size);
int process_relocs(int fd, struct flat_hdr * header, void * dram, uint32_t dram_base, void * iram, uint32_t iram_base);
