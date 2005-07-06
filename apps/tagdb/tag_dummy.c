#include "config.h"
#include "malloc.h"

#include "tag_dummy.h"
#include <string.h>

int tag_dummy(char *file, struct tag_info *t) {
	t->song = malloc(strlen(file)+1);
	strcpy(t->song, file);
	return ERR_NONE;
}
