
#ifndef _GNUCHESS_H_
#define _GNUCHESS_H_

#define neutral 2
#define white 0
#define black 1 
#define no_piece 0
#define pawn 1
#define knight 2
#define bishop 3
#define rook 4
#define queen 5
#define king 6
#define valueP 100
#define valueN 350
#define valueB 355
#define valueR 550
#define valueQ 1100
#define valueK 1200

/* ---- chess system global variables ---- */
extern short mate,opponent,computer;
extern short locn[8][8];
extern short board[64];
extern short color[64];
extern long Level;
extern short TCflag,TCmoves,TCminutes;
extern short timeout;

/* ---- RockBox integration ---- */
extern struct plugin_api* rb;

/* ---- The beginning of a GNUChess v2 APIfication ---- */
void SetTimeControl(void);
void GNUChess_Initialize(void);
int  VerifyMove(char s[],short iop,unsigned short *mv);
int  SelectMove ( short side, short iop , void (*callback)(void) );

#endif
