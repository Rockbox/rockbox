#ifndef DSP_IMAGE_HELLOWORLD
#define DSP_IMAGE_HELLOWORLD
/*
 * This is just a dummy DSP image so that dsp-dm320.c compiles.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

static const struct dsp_section dsp_image_helloworld[] = {
	{NULL, 0, 0}
};

/* Symbol table, usable with the DSP_() macro (see dsp-target.h). */
#define _status 0x0000
#define _acked  0x0000

#endif
