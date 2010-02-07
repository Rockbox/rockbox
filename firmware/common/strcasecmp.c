
#include <string.h>
#include <ctype.h>

int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
    }

    return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

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
