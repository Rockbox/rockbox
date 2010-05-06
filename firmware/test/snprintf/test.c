
#include <stdio.h>


int main(int argc, char **argv)
{
    char buffer[256];
    snprintf(buffer, 5, "123456789");
    printf("%s\n", buffer);
    snprintf(buffer, 6, "123456789");
    printf("%s\n", buffer);
    snprintf(buffer, 7, "123456789");
    printf("%s\n", buffer);
    snprintf(buffer, 7, "123%s", "mooooooooooo");
    printf("%s\n", buffer);
}
