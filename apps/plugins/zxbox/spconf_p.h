/* 
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef SPCONF_P_H
#define SPCONF_P_H

#include "spconf.h"

struct sp_options {
  const char *option;

  int argtype;
  void *argvalp;
  const char **enums;
  int disp;
};

extern struct sp_options spcf_options[];

#define SA_BOOL     1
#define SA_INT      2
#define SA_STR      3
#define SA_ENUM     4


extern int showframe;
extern int scrmul;
extern int privatemap;
extern int use_shm;

extern int small_screen;

extern int sound_on;
extern int bufframes;
extern const char *sound_dev_name;
extern int sound_sample_rate;
extern int sound_to_autoclose;
extern int sound_dsp_setfrag;

extern int keyboard_type;
extern int cursor_type;
extern int color_type;
extern int pause_on_iconify;
extern int vga_pause_bg;
extern int sp_quick_load;
extern int sp_paused;
extern int load_immed;
extern int spt_auto_stop;
extern int spkb_allow_ascii;
extern int spkb_trueshift;
extern int spkb_funcshift;

extern void spcf_set_val(int ix, const char *val, const char *name, 
             int ctr, int fatal);

extern void spcf_set_color(int ix, const char *val, const char *name, 
               int ctr, int fatal);
extern void spcf_set_key(int ix, const char *val, const char *name,
             int ctr, int fatal);
extern int spcf_match_keydef(const char *attr, const char *beg);
extern int spcf_parse_conf_line(char *line, char **attrp, char **valp);


#endif /* SPCONF_P_H */
