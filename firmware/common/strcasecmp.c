
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
    if(!n)
      return 0;

    while (n-- != 0 && tolower(*s1) == tolower(*s2)) {
        if(n == 0 || *s1 == '\0')
          break;
        s1++;
        s2++;
    }

    return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}
