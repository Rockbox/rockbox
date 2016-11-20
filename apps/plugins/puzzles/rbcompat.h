#include "plugin.h"
#include "rbassert.h"

int sprintf_wrapper(char *str, const char *fmt, ...);
char *getenv_wrapper(const char *c);
int puts_wrapper(const char *s);
double sin_wrapper(double rads);
double cos_wrapper(double rads);
int vsprintf_wrapper(char *s, const char *fmt, va_list ap);
double fabs_wrapper(double n);
double floor_wrapper(double n);

double atan_wrapper(double x);
double atan2_wrapper(double y, double x);
double sqrt_wrapper(double x);
long strtol_wrapper(const char *nptr, char **endptr, int base);
int64_t strtoq_wrapper(const char *nptr, char **endptr, int base);
uint64_t strtouq_wrapper(const char *nptr, char **endptr, int base);
double pow_wrapper(double x, double y);
double scalbn_wrapper (double x, int n);
double ceil_wrapper(double x);

size_t strspn_wrapper(const char *s1, const char *s2);
size_t strcspn_wrapper(const char *s1, const char *s2);
int sscanf_wrapper(const char *ibuf, const char *fmt, ...);
double atof_wrapper(char *s);
double acos_wrapper(double x);


#define acos acos_wrapper
#define atan atan_wrapper
#define atan2 atan2_wrapper
#define atof atof_wrapper
#define atoi rb->atoi
#define atol atoi
#define calloc tlsf_calloc
#define ceil ceil_wrapper
#define cos cos_wrapper
#define fabs fabs_wrapper
#define floor floor_wrapper
#define free tlsf_free
#define getenv getenv_wrapper
#define malloc tlsf_malloc
#define memchr rb->memchr
#define pow pow_wrapper
#define printf LOGF
#define puts puts_wrapper
#define qsort rb->qsort
#define realloc tlsf_realloc
#define sin sin_wrapper
#define sprintf sprintf_wrapper
#define sqrt sqrt_wrapper
#define sscanf sscanf_wrapper
#define strcat rb->strcat
#define strchr rb->strchr
#define strcmp rb->strcmp
#define strcpy rb->strcpy
#define strcspn strcspn_wrapper
#define strlen rb->strlen
#define strspn strspn_wrapper
#define strtol strtol_wrapper
#define strtoq strtoq_wrapper
#define strtouq strtouq_wrapper
#define vsprintf vsprintf_wrapper
