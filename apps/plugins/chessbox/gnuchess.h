
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

/* ---- chess engine global types ---- */
struct GameRec {
    unsigned short gmove;
    short score,depth,time,piece,color;
    long nodes;
};
struct TimeControlRec {
    short moves[2];
    long clock[2];
};

/* ---- chess engine global variables ---- */
extern short mate,opponent,computer,Sdepth;
extern short locn[8][8];
extern short board[64];
extern short color[64];
extern bool withbook;
extern long Level;
extern short TCflag,TCmoves,TCminutes;
extern short timeout;
extern short GameCnt,Game50,castld[2],kingmoved[2],OperatorTime;
extern struct TimeControlRec TimeControl;
extern struct GameRec GameList[240];

/* ---- The beginning of a GNUChess v2 APIfication ---- */
void SetTimeControl(void);
void GNUChess_Initialize(void);
int  VerifyMove(short player, char s[],short iop,unsigned short *mv, char *move_buffer);
int  SelectMove ( short side, short iop , void (*callback)(void), char *move_buffer );
void InitializeStats ( void );
void ElapsedTime ( short iop );

#endif
