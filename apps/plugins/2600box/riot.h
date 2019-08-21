
#ifndef RIOT_H
#define RIOT_H


void init_riot(void);

unsigned read_riot(unsigned a);
void write_riot(unsigned a, unsigned b);

struct riot {
    unsigned timer_res;
    unsigned timer_count;
    unsigned timer_end;
    unsigned timer_clks;
    BYTE read[0x20];
    BYTE write[0x20];
};

extern struct riot riot;

extern BYTE do_keypad(int pad, int col);

#endif /* RIOT_H */
