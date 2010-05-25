#include "skin_parser.h"
#include "skin_debug.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    char* doc = "This is a sample %V(1, 2, 3, 4, 5, six, seven)\n"
                "WPS document, with ; sublines and a %?T(conditional| or| two)";

    struct skin_element* test = skin_parse(doc);

    skin_debug_tree(test);

    return 0;
}
