/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_vars.c: global refresh variables

#include	"quakedef.h"

#if	!id386

// all global and static refresh variables are collected in a contiguous block
// to avoid cache conflicts.

//-------------------------------------------------------
// global refresh variables
//-------------------------------------------------------

// FIXME: make into one big structure, like cl or sv
// FIXME: do separately for refresh engine and driver

float	d_sdivzstepu, d_tdivzstepu, d_zistepu;
float	d_sdivzstepv, d_tdivzstepv, d_zistepv;
float	d_sdivzorigin, d_tdivzorigin, d_ziorigin;
#ifdef USE_PQ_OPT3
int		d_sdivzstepu_fxp, d_tdivzstepu_fxp, d_zistepu_fxp;
int		d_sdivzstepv_fxp, d_tdivzstepv_fxp, d_zistepv_fxp;
int		d_sdivzorigin_fxp, d_tdivzorigin_fxp, d_ziorigin_fxp;
#endif

int d_ziorigin_fxp, d_zistepv_fxp, d_zistepu_fxp;

#ifdef USE_PQ_OPT
//JB: Optimization
int sdivzstepu, tdivzstepu, zistepu;
int sdivzstepv, tdivzstepv, zistepv;
int sdivzorigin, tdivzorigin, ziorigin;
#endif

#ifdef USEFPM
fixedpoint_t	d_sdivzstepuFPM, d_tdivzstepuFPM, d_zistepuFPM;
fixedpoint_t	d_sdivzstepvFPM, d_tdivzstepvFPM, d_zistepvFPM;
fixedpoint_t	d_sdivzoriginFPM, d_tdivzoriginFPM, d_zioriginFPM;
#endif

fixed16_t	sadjust, tadjust, bbextents, bbextentt;

pixel_t			*cacheblock;
int				cachewidth;
pixel_t			*d_viewbuffer;
short			*d_pzbuffer;
unsigned int	d_zrowbytes;
unsigned int	d_zwidth;

#endif	// !id386

