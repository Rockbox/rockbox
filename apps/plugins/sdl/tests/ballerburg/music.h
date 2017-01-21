/*
    music.h - prototypes and definitions for music.c

    Copyright (C) 2011  Thomas Huth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

void m_laden(const char * string);
void m_quit(void);
void m_musik(void);
void m_wloop(void);
void s_note(unsigned int wert);
void s_init(void);
void s_quit(void);
void s_freq(unsigned short freq);
void s_laut(short laut);
void s_rausch(short periode);
void s_t_an(void);
void s_t_aus(void);
void s_r_an(void);
void s_r_aus(void);
void s_aus(void);
