

#ifndef TIA_VIDEO_H
#define TIA_VIDEO_H


void init_tia (void);


unsigned read_tia(unsigned a);
void write_tia(unsigned a, unsigned b);

// to be removed... tiaWrite is used atm for do_paddle and keyboard.
extern BYTE tiaWrite[0x40];
extern BYTE tiaRead[0x10];

extern unsigned tia_delay;

#endif
