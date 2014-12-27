#include "unit.h"
void temp_conv(struct unit_t *units, int from, int to)
{
    /* convert everything to kelvin */
    data_t kelvin;
    switch(from)
    {
    case 0:
        /* kelvin */
        kelvin = units[from].value;
        break;
    case 1:
        /* celsius */
        kelvin = units[from].value + 273.15 * UNIT;
        break;
    case 2:
        /* fahrenheit */
        kelvin = (units[from].value + (459.67 * UNIT)) * (5.0/9.0);
        break;
    case 3:
        /* rankine */
        kelvin = units[from].value * (5.0/9.0);
        break;
    case 4:
        /* delisle */
        kelvin = UNIT * 373.15 - units[from].value * (2.0/3.0);
        break;
    case 5:
        /* newton */
        kelvin = units[from].value * (100.0/33.0) + (273.15 * UNIT);
        break;
    case 6:
        /* réaumur */
        kelvin = units[from].value * 1.25 + (273.15 * UNIT);
        break;
    }
    switch(to)
    {
    case 0:
        units[to].value = kelvin;
        break;
    case 1:
        /* celsius */
        units[to].value = kelvin - 273.15 * UNIT;
        break;
    case 2:
        /* fahrenheit */
        units[to].value = kelvin * 1.8 - 459.67 * UNIT;
        break;
    case 3:
        /* rankine */
        units[to].value = kelvin * 1.8;
        break;
    case 4:
        /* delisle */
        units[to].value = 1.5 * (373.15 * UNIT - kelvin);
        break;
    case 5:
        /* newton */
        units[to].value = (kelvin - 273.15 * UNIT) * 33.0/100.0;
        break;
    case 6:
        /* réaumur */
        units[to].value = (kelvin - 273.15 * UNIT) * 0.8;
        break;
    }
}
