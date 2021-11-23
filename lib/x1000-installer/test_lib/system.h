#ifndef SYSTEM_H
#define SYSTEM_H

#define CACHEALIGN_SIZE 1
#define CACHEALIGN_BUFFER(x,y) do { } while(0)
#define MIN(a, b) (((a)<(b))?(a):(b))
#define ALIGN_BUFFER(ptr, size, align) do { } while(0)
#define ALIGN_UP_P2(x, p) (((x) + ((1 << (p)) - 1)) & ~((1 << (p)) - 1))

#endif
