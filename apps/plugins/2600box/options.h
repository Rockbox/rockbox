#ifndef OPTIONS_H
#define OPTIONS_H

enum {NTSC=0, PAL=1, SECAM=2};

/* Options common to all ports of x2600 */
extern struct BaseOptions {
  int rr;
  int tvtype;
  int lcon;
  int rcon;
  int bank;
  int magstep;
  char filename[80];
  int sound;
  int swap;
  int realjoy;
  int limit;
  int mousey;
  int mitshm;
  int dbg_level;
} base_opts;

int 
parse_options(char *file);

#endif
