#include "config.h"
#include "malloc.h"

#undef malloc
#undef free
#undef realloc

#undef DEBUGF
#define DEBUGF(...)

#include <stdio.h>
#include <stdlib.h>

static size_t total=0;
static size_t max_total=0;

struct size_array {
	void *ptr;
	size_t size;
} sizes[1000];
#define NOT_FOUND 1001
static unsigned long count=0;

int out_of_memory = 1000000;

void *do_malloc(size_t size) {
	void *ret;
	if(total + size > out_of_memory) {
		DEBUGF("malloc(%d), total=%d: FAILED: simulating out-of-memory\n", size, total+size);
		return NULL;
	}
	
	ret = malloc(size);
	if( ret == NULL ) {
		DEBUGF("malloc(%d), total=%d FAILED\n", size, total+size);
		return NULL;
	} else {
		total += size;
		max_total = ( total > max_total ? total : max_total );
		sizes[count].ptr = ret;
		sizes[count].size = size;
		DEBUGF("malloc(%d), total=%d OK => 0x%08lx (%lu)\n", size, total, (unsigned long)ret, count);
		count++;
		if(count == NOT_FOUND) {
			fprintf(stderr, "MALLOC MEMORY FULL!!!!!!! FAILING\n");
			free(ret);
			count--;
			return NULL;
		}
		return ret;
	}
}

static unsigned long find(void* ptr) {
	unsigned long i;
	for(i=0; i<count; i++) {
		if( ptr == sizes[i].ptr ) {
			return i;
		}
	}
	return NOT_FOUND;
}

void do_free(void *ptr) {
	unsigned long i;

	i = find(ptr);
	if( i == NOT_FOUND ) {
		DEBUGF("free(%08lx) (?) ptr unknown\n", (unsigned long)ptr);
		free(ptr);
	} else {
		total -= sizes[i].size;
		DEBUGF("free(%08lx) (%lu, %dbytes) => total=%u\n", (unsigned long)ptr, i, sizes[i].size, total);
		free(ptr);
		sizes[i].ptr = NULL; // delete
		sizes[i].size = 0;
	}
}

void *do_realloc(void *ptr, size_t size) {
	void *ret;
	unsigned long i;
	
	if( ptr == NULL ) {
		DEBUGF("realloc()=>");
		return do_malloc(size);
	}
	
	i = find(ptr);

	if( i == NOT_FOUND ) {
		DEBUGF("realloc(%08lx, %d) (?) ptr unknown ", (unsigned long)ptr, size);
	} else {
		DEBUGF("realloc(%08lx, %d) (%lu, %dbytes) => total=%d ", (unsigned long)ptr, size, i, sizes[i].size, total+size-sizes[i].size);
	}
	
	if(total + size - sizes[i].size > out_of_memory) {
		DEBUGF("FAILED: simulating out-of-memory\n");
		return NULL;
	}
	
	ret = realloc(ptr, size);
	if( ret == NULL && size != 0) { // realloc(x, 0) returns NULL, but is valid!
		DEBUGF("FAILED\n");
	} else {
		total += size - sizes[i].size;
		max_total = ( total > max_total ? total : max_total );
		sizes[i].ptr = ret;	// update the ptr if realloc changed it
		sizes[i].size = size;
		DEBUGF("=> %08lx\n", (unsigned long)ret);
	}
	return ret;
}

void malloc_stats() {
	unsigned long i, j;
	
	printf("malloc stats:\n");
	printf("  Total number of allocated items:    %lu\n", count);
	printf("  Current number of allocated items:  ");
	j=0;
	for(i=0; i<count; i++) {
		if( sizes[i].ptr != NULL) {
			printf("%lu ", i);
			j++;
		}
	}
	printf("=> %lu items\n", j);
	printf("  Maximum amount of allocated memory: %dbytes\n", max_total);
	printf("  Current amount of allocated memory: %dbytes\n", total);
}
