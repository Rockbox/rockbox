#include "unique.h"

#include <string.h>
#include <stdio.h>

char *create_unique_name(char *buffer, const char *prefix, const char *suffix, int digits) {
	static unsigned long i=0;

	strcpy(buffer, prefix);
	sprintf(buffer+strlen(prefix), "%05lu", i);
	strcat(buffer, suffix);

	i++;

	return buffer;
}
