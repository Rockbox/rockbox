#ifndef DEBUG_H
#define DEBUG_H

void dbg_dump_sector(int sec);
void dbg_dump_buffer(unsigned char *buf);
void dbg_print_bpb(struct bpb *bpb);
void dbg_console(struct bpb *bpb);

#endif
