/* 
 * Copyright (C) 1996-1998 Szeredi Miklos 2006 Anton Romanov
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

#include "zxconfig.h"

#include "sptiming.h"
#include "interf.h"
/* not precise but .... anyway this is used only when not playing
 * sound ... */
long shouldbe_tick;

void spti_init(void){
	spti_reset();
}
void spti_sleep(unsigned long usecs){
/*	unsigned long now,need;
	now = *rb->current_tick;
	need = now + usecs;
	rb -> sleep ( need - now );*/
    rb->sleep ( usecs );
}
void spti_reset(void){
	shouldbe_tick = *rb -> current_tick;
}
void spti_wait(void){
	long rem;
	long now;
	
	now = *rb -> current_tick;
    shouldbe_tick+=SKIPTICKS;
	rem = shouldbe_tick - now;

	if(rem > 0) {
		if(rem > SKIPTICKS) rem = SKIPTICKS;
			spti_sleep((unsigned long) rem);
	}
    if(rem == SKIPTICKS || rem < -10 * SKIPTIME) spti_reset();
}
