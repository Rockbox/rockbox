#ifndef DBG_MESS_H
#define DBG_MESS_H

enum dbg_level {DBG_NONE=0, DBG_USER=1, DBG_NORMAL=2, DBG_LOTS=3, DBG_OBSCENE=4};

void 
dbg_message(int level, const char *format, ... );

#endif
