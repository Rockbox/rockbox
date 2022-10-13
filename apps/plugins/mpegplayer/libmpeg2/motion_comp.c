/*
 * motion_comp.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 * libmpeg2 sync history:
 * 2008-07-01 - CVS revision 1.17 - lost compatibility previously to
 *              provide simplified and CPU-optimized motion compensation.
 */

#include "plugin.h"

#include "mpeg2dec_config.h"

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

/* These are defined in their respective target files - motion_comp_X.c */
extern mpeg2_mc_fct MC_put_o_16;
extern mpeg2_mc_fct MC_put_o_8;
extern mpeg2_mc_fct MC_put_x_16;
extern mpeg2_mc_fct MC_put_x_8;
extern mpeg2_mc_fct MC_put_y_16;
extern mpeg2_mc_fct MC_put_y_8;
extern mpeg2_mc_fct MC_put_xy_16;
extern mpeg2_mc_fct MC_put_xy_8;

extern mpeg2_mc_fct MC_avg_o_16;
extern mpeg2_mc_fct MC_avg_o_8;
extern mpeg2_mc_fct MC_avg_x_16;
extern mpeg2_mc_fct MC_avg_x_8;
extern mpeg2_mc_fct MC_avg_y_16;
extern mpeg2_mc_fct MC_avg_y_8;
extern mpeg2_mc_fct MC_avg_xy_16;
extern mpeg2_mc_fct MC_avg_xy_8;

const mpeg2_mc_t mpeg2_mc =
{
    {
        MC_put_o_16, MC_put_x_16, MC_put_y_16, MC_put_xy_16,
        MC_put_o_8,  MC_put_x_8,  MC_put_y_8,  MC_put_xy_8
    },
    {
        MC_avg_o_16, MC_avg_x_16, MC_avg_y_16, MC_avg_xy_16,
        MC_avg_o_8,  MC_avg_x_8,  MC_avg_y_8,  MC_avg_xy_8
    }
};
