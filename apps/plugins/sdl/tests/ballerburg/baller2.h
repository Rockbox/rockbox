/*
    baller2.h - prototypes and definitions for baller2.c

    Copyright (C) 2010  Thomas Huth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

int comp(void);
void z_kn(void);
void z_ka(void);
void z_ft(void);
void z_ge(void);
void z_pk(void);
void schuss(int k);
void expls(int x, int y, int w, int h, int d);
int kugel(int x, int y);
void baller(unsigned char r);
void bild(void);
void burg(int nn);
void init_ka(int k, int xr);
void drw_all(void);
void drw_gpk(char w);
void draw(short x, short y, short *a);
void fturm(void);
void koenig(void);
int drin(short xk, short yk, short w, short h, short r, short x, short y);
