

#ifndef __RTC_GB_H__
#define __RTC_GB_H__


struct rtc
{
	int batt;
	int sel;
	int latch;
	int d, h, m, s, t;
	int stop, carry;
	byte regs[8];
};

extern struct rtc rtc;

void rtc_latch(byte b);
void rtc_write(byte b);
void rtc_tick(void);
void rtc_save_internal(int fd);
void rtc_load_internal(int fd);
	
#endif
