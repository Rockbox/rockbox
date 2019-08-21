

#ifndef TIA_VIDEO_H
#define TIA_VIDEO_H


void init_tia (void);


unsigned read_tia(unsigned a);
void write_tia(unsigned a, unsigned b);

// to be removed... tiaWrite is used atm for do_paddle and keyboard.
extern BYTE tiaWrite[0x40];
extern BYTE tiaRead[0x10];

/* Note: the non-table version leads to jumpy lines with some ROMs */
/* edit: seems to be fixed now. Non-Table-Version is slightly slower. */
#define USE_DIV_3_TABLE

#ifdef USE_DIV_3_TABLE
/* must be more than 228 because a cpu instruction can go up to 6 cycles beyond */
extern int  div3tab[250]; /* lookup table to convert pixel clock to cpu clock. Index 0 is pixel clock -68 */
extern int  div3pixel[250]; /* number of pixels I have to add to ebeamx */
#endif


#endif /* TIA_VIDEO_H */
