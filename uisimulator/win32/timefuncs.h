#include <stdio.h>
#include <time.h>
#include <stdbool.h>

/* struct tm defined */
struct tm *get_time(void);
int set_time(const struct tm *tm);
bool valid_time(const struct tm *tm);
