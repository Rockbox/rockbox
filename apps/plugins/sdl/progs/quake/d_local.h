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
// d_local.h:  private rasterization driver defs

#include "r_shared.h"

//
// TODO: fine-tune this; it's based on providing some overage even if there
// is a 2k-wide scan, with subdivision every 8, for 256 spans of 12 bytes each
//
#define SCANBUFFERPAD		0x1000

#define R_SKY_SMASK	0x007F0000
#define R_SKY_TMASK	0x007F0000

#define DS_SPAN_LIST_END	-128

#define SURFCACHE_SIZE_AT_320X200	600*1024

typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s	*texture;	// checked for animating textures
	byte				data[4];	// width*height elements
} surfcache_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct sspan_s
{
	int				u, v, count;
} sspan_t;

extern cvar_t	d_subdiv16;

extern float	scale_for_mip;

extern qboolean		d_roverwrapped;
extern surfcache_t	*sc_rover;
extern surfcache_t	*d_initial_rover;

extern float	d_sdivzstepu, d_tdivzstepu, d_zistepu;
extern float	d_sdivzstepv, d_tdivzstepv, d_zistepv;
extern float	d_sdivzorigin, d_tdivzorigin, d_ziorigin;

extern int		d_zistepu_fxp, d_zistepv_fxp, d_ziorigin_fxp;

#ifdef USE_PQ_OPT3
extern int		d_sdivzstepu_fxp, d_tdivzstepu_fxp, d_zistepu_fxp;
extern int		d_sdivzstepv_fxp, d_tdivzstepv_fxp, d_zistepv_fxp;
extern int		d_sdivzorigin_fxp, d_tdivzorigin_fxp, d_ziorigin_fxp;
#endif


#ifdef USE_PQ_OPT
//JB:Optimization
//FW: disable
//extern int sdivzstepu, tdivzstepu, zistepu;
//extern int sdivzstepv, tdivzstepv, zistepv;
//extern int sdivzorigin, tdivzorigin, ziorigin;
#endif

extern fixedpoint_t	d_sdivzstepuFPM, d_tdivzstepuFPM, d_zistepuFPM;
extern fixedpoint_t	d_sdivzstepvFPM, d_tdivzstepvFPM, d_zistepvFPM;
extern fixedpoint_t	d_sdivzoriginFPM, d_tdivzoriginFPM, d_zioriginFPM;

//Dan: ID Software was already using a minute amount of fixed point.  I duplicated
//these just for consistancy in the conversion, and so the types would match.
fixed16_t		sadjust, tadjust;
fixed16_t		bbextents, bbextentt;
fixedpoint_t	sadjustFPM, tadjustFPM;
fixedpoint_t	bbextentsFPM, bbextenttFPM;


void D_DrawSpans8 (espan_t *pspans);
#ifdef USE_PQ_OPT
//JB:Optimization
void D_DrawSpans8WithZ (espan_t *pspans);
#endif
void D_DrawSpans8FPM (espan_t *pspans);
void D_DrawSpans16 (espan_t *pspans);
void D_DrawZSpans (espan_t *pspans);
void Turbulent8 (espan_t *pspan);
void D_SpriteDrawSpans (sspan_t *pspan);

void D_DrawSkyScans8 (espan_t *pspan);
void D_DrawSkyScans16 (espan_t *pspan);

void R_ShowSubDiv (void);
void (*prealspandrawer)(void);
surfcache_t	*D_CacheSurface (msurface_t *surface, int miplevel);

extern int D_MipLevelForScale (float scale);

#if id386
extern void D_PolysetAff8Start (void);
extern void D_PolysetAff8End (void);
#endif

extern short *d_pzbuffer;
extern unsigned int d_zrowbytes, d_zwidth;

extern int	*d_pscantable;
extern int	d_scantable[MAXHEIGHT];

extern int	d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle;

extern int	d_y_aspect_shift, d_pix_min, d_pix_max, d_pix_shift;

extern pixel_t	*d_viewbuffer;

extern short	*zspantable[MAXHEIGHT];

extern int		d_minmip;
extern float	d_scalemip[3];

extern void (*d_drawspans) (espan_t *pspan);

