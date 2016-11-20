
/*backlight_sel.h*/
#ifndef _SELBL_H_
#define _SELBL_H_

void set_selective_backlight_actions(bool selective, int mask, bool filter_fkp);
bool get_selective_backlight_status(void);
int get_selective_backlight_mask(void);
bool get_selective_backlight_filter_fkp(void);

#endif  /* _SELBL_H_ */

