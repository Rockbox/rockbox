#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "map.h"

const char *pin_group_color(unsigned group, unsigned block)
{
    (void)block;
    switch(group)
    {
        case PIN_GROUP_EMI: return "RED";
        case PIN_GROUP_GPIO: return "TEAL";
        case PIN_GROUP_I2C: return "PURPLE";
        case PIN_GROUP_JTAG: return "RED";
        case PIN_GROUP_PWM: return "OLIVE";
        case PIN_GROUP_SPDIF: return "OLIVE";
        case PIN_GROUP_TIMROT: return "PINK";
        case PIN_GROUP_AUART: return "GREEN";
        case PIN_GROUP_ETM: return "RED";
        case PIN_GROUP_GPMI: return "ORANGE";
        case PIN_GROUP_IrDA: return "OLIVE";
        case PIN_GROUP_LCD: return "TEAL";
        case PIN_GROUP_SAIF: return "ORANGE";
        case PIN_GROUP_SSP: return "PURPLE";
        case PIN_GROUP_DUART: return "GRAY";
        case PIN_GROUP_USB: return "LIME";
        case PIN_GROUP_NONE: return NULL;
        default: return NULL;
    }
}

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        printf("usage: %s <soc> <ver>\n", argv[0]);
        printf("  where <soc> is stmp3700 or imx233\n");
        printf("  where <ver> is bga169 or lqfp128\n");
        return 1;
    }

    const char *soc = argv[1];
    const char *ver = argv[2];

    struct bank_map_t *map = NULL;
    for(unsigned i = 0; i < NR_SOCS; i++)
        if(strcmp(soc, socs[i].soc) == 0 && strcmp(ver, socs[i].ver) == 0)
            map = socs[i].map;
    if(map == NULL)
    {
        printf("no valid map found\n");
        return 4;
    }

    for(unsigned bank = 0; bank < NR_BANKS; bank++)
    {
        for(unsigned offset = 0; offset < 2; offset ++)
        {
            printf("| *Bank %d* |", bank);
            for(unsigned count = 0; count < 16; count++)
                printf(" *%d* ||", offset * 16 + 15 - count);
            printf("\n");
            printf("| *Mux Reg %d* |", bank * 2 + offset);
            for(unsigned count = 0; count < 32; count++)
                printf(" *%d* |", 31 - count);
            printf("\n");

            for(unsigned function = 0; function < NR_FUNCTIONS; function++)
            {
                printf("| *select = %d* |", function);
                for(unsigned count = 0; count < 16; count++)
                {
                    unsigned pin_nr = offset * 16 + 15 - count;
                    struct pin_function_desc_t *desc = &map[bank].pins[pin_nr].function[function];
                    const char *color = pin_group_color(desc->group, desc->block);
                    printf(" ");
                    if(color)
                        printf("%%%s%%", color);
                    if(desc->name)
                        printf("%s", desc->name);
                    if(color)
                        printf("%%ENDCOLOR%%");
                    printf(" ||");
                }
                printf("\n");
            }

            printf("| |");
            for(unsigned count = 0; count < 16; count++)
                printf("||");
            printf("\n");
        }
    }

    return 0;
}
 
