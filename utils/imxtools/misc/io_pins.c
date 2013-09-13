#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "map.h"

bool read_number(const char *str, uint32_t *value)
{
    char *end;
    long int ret = strtol(str, &end, 0);
    if((str + strlen(str)) != end)
        return false;
    if(ret < 0)
        return false;
    *value = ret;
    return true;
}

int main(int argc, char **argv)
{
    if(argc != 6)
    {
        printf("usage: %s <soc> <ver> <mux> <clr mask> <set mask>\n", argv[0]);
        printf("  where <soc> is stmp3700 or imx233\n");
        printf("  where <ver> is bga169 or lqfp128\n");
        printf("  where <mux> is between 0 and 7");
        printf("  where <mask> is a number or ~number\n");
        return 1;
    }

    const char *soc = argv[1];
    const char *ver = argv[2];
    const char *s_mux = argv[3];
    const char *s_clr_mask = argv[4];
    const char *s_set_mask = argv[5];
    uint32_t mux, clr_mask, set_mask;

    if(!read_number(s_mux, &mux) || mux >= NR_BANKS * 2)
    {
        printf("invalid mux number\n");
        return 1;
    }
    if(!read_number(s_clr_mask, &clr_mask))
    {
        printf("invalid clear mask\n");
        return 2;
    }
    if(!read_number(s_set_mask, &set_mask))
    {
        printf("invalid set mask\n");
        return 3;
    }

    struct bank_map_t *map = NULL;
    for(unsigned i = 0; i < NR_SOCS; i++)
        if(strcmp(soc, socs[i].soc) == 0 && strcmp(ver, socs[i].ver) == 0)
            map = socs[i].map;
    if(map == NULL)
    {
        printf("no valid map found\n");
        return 4;
    }

    if(clr_mask & set_mask)
        printf("warning: set and clear mask intersect!\n");
    unsigned bank = mux / 2;
    unsigned offset = 16 * (mux % 2);
    for(unsigned i = 0; i < 16; i++)
    {
        unsigned pin = offset + i;
        uint32_t pin_shift = 2 * i;
        uint32_t set_fn = (set_mask >> pin_shift) & 3;
        uint32_t clr_fn = (clr_mask >> pin_shift) & 3;
        if(set_fn == 0 && clr_fn == 0)
            continue;
        bool partial_mask = (set_fn | clr_fn) != 3;
        
        printf("B%dP%02d => %s (select = %d)", bank, pin,
            map[bank].pins[pin].function[set_fn].name, set_fn);
        if(partial_mask)
            printf(" (warning: partial mask)");
        printf("\n");
    }
    
    return 0;
}
