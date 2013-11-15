#ifndef REALJOY_H
#define REALJOY_H

#define LJOYMASK 0x01
#define RJOYMASK 0x02
#define UJOYMASK 0x04
#define DJOYMASK 0x08
#define B1JOYMASK 0x10
#define B2JOYMASK 0x20

/* This should be called from xxx_keyb.c */
int
get_realjoy(int stick);

/* This should be called once per frame */
void 
update_realjoy(void);

/* These can be called from anywhere. */
void
calibrate_realjoy(int stick);

void
init_realjoy(void);

void
close_realjoy(void);

#endif
