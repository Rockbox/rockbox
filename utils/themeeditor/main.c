#include "skin_parser.h"
#include "skin_debug.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    char* doc = "%Vd(U))\n\n%?bl(test,3,5,2,1)<param2|param3>";

    struct skin_element* test = skin_parse(doc);

    skin_debug_tree(test);

    return 0;
}
