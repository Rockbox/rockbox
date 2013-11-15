#ifndef _VIDMODE
#define _VIDMODE

int
get_nummodes(void);

char *
describe_mode(int m);

void
modeswitch(int mode);

int
modeswitch_on(void);

void
modeswitch_off(void);

void
position_port(int x, int y);

extern int modeswitch_flag;

#endif
