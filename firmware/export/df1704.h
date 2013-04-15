
#ifndef _DF1704_H
#define _DF1704_H

#define	VOLUME_MIN	0
#define	VOLUME_MAX	-730

int df1704_init(void);
void df1704_pwren(unsigned char en);
void amp_pwren(unsigned char en);
void df1704_mute(void);
void df1704_unmute(void);
void df1704_setVol(int Volume);

void audiohw_set_volume(int v);

#endif
