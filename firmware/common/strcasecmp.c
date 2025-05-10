
#include <string.h>
#include <ctype.h>

#ifndef strcasecmp
int strcasecmp(const char *s1, const char *s2)
{
    int d, c1, c2;
    do
    {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    }
    while ((d = c1 - c2) == 0 && c1 && c2);
    return d;
}
#endif

#ifndef strncasecmp
int strncasecmp(const char *s1, const char *s2, size_t n)
{
    int d = 0;
    
    for(; n != 0; n--)
    {
        int c1 = tolower(*s1++);
        int c2 = tolower(*s2++);
        if((d = c1 - c2) != 0 || c2 == '\0')
            break;
    }
    
    return d;
}
#endif
