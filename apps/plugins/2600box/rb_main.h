

#ifndef RB_MAIN_H
#define RB_MAIN_H


#ifndef ATARI_NO_SOUND
extern struct event_queue audiosync SHAREDBSS_ATTR;
#endif


/*
 * functions to pause/resume emulation are used when menu is entered/left
 */
void emulation_pause(void);
void emulation_resume(void);


#endif /* RB_MAIN_H */
