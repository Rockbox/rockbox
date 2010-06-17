/*
 * (C) Copyright 2007 Catalin Patulea <cat@vv.carleton.ca>
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
#ifndef DSP_H
#define DSP_H

/* DSP memory is mapped into ARM space via HPIB. */
#define DSP_(addr) (*(volatile unsigned short *)(0x40000 + ((addr) << 1)))

/* A "DSP image" is an array of these, terminated by raw_data_size_half = 0. */
struct dsp_section {
    const unsigned short *raw_data;
    unsigned short physical_addr;
    unsigned short raw_data_size_half;
};

#define dsp_message (*(volatile struct ipc_message *)&DSP_(_status))

/* Must define struct dsp_section before including the image. */
#include "dsp/dsp-image.h"

void dsp_wake(void);
void dsp_load(const struct dsp_section *im);
void dsp_reset(void);

#endif
