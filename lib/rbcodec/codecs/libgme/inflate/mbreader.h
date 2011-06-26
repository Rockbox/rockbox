
#ifndef MBREADER_H
#define MBREADER_H

#include "codeclib.h"

struct mbreader_t {
	const char *ptr;
	size_t size;
	size_t offset;
};

int mbread(struct mbreader_t *md, void *buf, size_t n);

#endif
