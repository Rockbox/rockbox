#include <stdint.h>
#include <stdio.h>

#include "common.h"

uint32_t filesize(FILE * f)
{
    uint32_t filesize;

    fseek(f, 0, SEEK_END);
    filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    return filesize;
}

