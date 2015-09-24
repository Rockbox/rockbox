#include "plugin.h"

#define DUCKY_MAGIC 0x4475634B /* DucK */

#define MAXOPSTACK 64
#define MAXNUMSTACK 64
#define CALL_STACK_SZ 64
#define VARMAP_SIZE 16

#define VARFORMAT "%d"
#define VARNAME_MAX 24

typedef int32_t imm_t;
typedef uint8_t instr_t;
typedef uint16_t varid_t;
typedef imm_t vartype;

extern int file_des, keys_sent;
extern int default_delay, string_delay;

void error(const char *fmt, ...) __attribute__((noreturn)) ATTRIBUTE_PRINTF(1,2);
void vid_write(const char *str);
void vid_writef(const char *fmt, ...) ATTRIBUTE_PRINTF(1,2);
imm_t read_imm(void);
void ducky_vm(int);
void send(int);
void send_string(const char *);
void add_char(int *keystate, unsigned *nkeys, char c);
void add_key(int *keystate, unsigned *nkeys, int);
void __attribute__((format(printf,1,2))) vid_writef_no_nl(const char *fmt, ...);
