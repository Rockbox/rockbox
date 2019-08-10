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
// r_main.c

#include "quakedef.h"
#include "r_local.h"

//define	PASSAGES

void		*colormap;
vec3_t		viewlightvec;
alight_t	r_viewlighting = {128, 192, viewlightvec};
float		r_time1;
int			r_numallocatededges;
qboolean	r_drawpolys;
qboolean	r_drawculledpolys;
qboolean	r_worldpolysbacktofront;
qboolean	r_recursiveaffinetriangles = true;
int			r_pixbytes = 1;
float		r_aliasuvscale = 1.0;
int			r_outofsurfaces;
int			r_outofedges;

qboolean	r_dowarp, r_dowarpold, r_viewchanged;

int			numbtofpolys;
btofpoly_t	*pbtofpolys;
mvertex_t	*r_pcurrentvertbase;
#ifdef USE_PQ_OPT2
mvertex_fxp_t	*r_pcurrentvertbase_fxp;
#endif

int			c_surf;
int			r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
qboolean	r_surfsonstack;
int			r_clipflags;

byte		*r_warpbuffer;

byte		*r_stack_start;

qboolean	r_fov_greater_than_90;

//
// view origin
//
vec3_t	vup, base_vup;
vec3_t	vpn, base_vpn;
vec3_t	vright, base_vright;
vec3_t	r_origin;
#ifdef USE_PQ_OPT1
int		vup_fxp[3];
int		vpn_fxp[3];
int		vright_fxp[3];
int		xscale_fxp, yscale_fxp;
int		xcenter_fxp, ycenter_fxp;
int		r_refdef_fvrectx_adj_fxp;
int		r_refdef_fvrectright_adj_fxp;
int		r_refdef_fvrecty_adj_fxp;
int		r_refdef_fvrectbottom_adj_fxp;
extern int		modelorg_fxp[3];
#endif

//
// screen size info
//
refdef_t	r_refdef;
float		xcenter, ycenter;
float		xscale, yscale;
float		xscaleinv, yscaleinv;
float		xscaleshrink, yscaleshrink;
float		aliasxscale, aliasyscale, aliasxcenter, aliasycenter;
fixedpoint_t	aliasxscaleFPM, aliasyscaleFPM, aliasxcenterFPM, aliasycenterFPM;
int		screenwidth;
#ifdef USE_PQ_OPT3
int xscaleinv_fxp, yscaleinv_fxp;
int	xcenter_fxp, ycenter_fxp;
#endif

float	pixelAspect;
float	screenAspect;
float	verticalFieldOfView;
float	xOrigin, yOrigin;

mplane_t	screenedge[4];

//
// refresh flags
//
int		r_framecount = 1;	// so frame counts initialized to 0 don't match
int		r_visframecount;
int		d_spanpixcount;
int		r_polycount;
int		r_drawnpolycount;
int		r_wholepolycount;

#define		VIEWMODNAME_LENGTH	256
char		viewmodname[VIEWMODNAME_LENGTH+1];
int			modcount;

int			*pfrustum_indexes[4];
int			r_frustum_indexes[4*6];

int		reinit_surfcache = 1;	// if 1, surface cache is currently empty and
								// must be reinitialized for current cache size

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

float		r_aliastransition, r_resfudge;

#ifdef USEFPM
mleaf_FPM_t	*r_viewleafFPM, *r_oldviewleafFPM;
mplane_FPM_t	screenedgeFPM[4];
fixedpoint_t	pixelAspectFPM;
fixedpoint_t	screenAspectFPM;
fixedpoint_t	verticalFieldOfViewFPM;
fixedpoint_t	xOriginFPM, yOriginFPM;
fixedpoint_t	aliasxscaleFPM, aliasyscaleFPM, aliasxcenterFPM, aliasycenterFPM;
fixedpoint_t	r_aliastransitionFPM, r_resfudgeFPM;
fixedpoint_t	aliasxscaleFPM, aliasyscaleFPM, aliasxcenterFPM, aliasycenterFPM;
fixedpoint_t	xscaleshrinkFPM, yscaleshrinkFPM;
fixedpoint_t	xscaleinvFPM, yscaleinvFPM;
fixedpoint_t	xscaleFPM, yscaleFPM;
fixedpoint_t	xcenterFPM, ycenterFPM;
refdef_FPM_t	r_refdefFPM;
vec3_FPM_t	vupFPM, base_vupFPM;
vec3_FPM_t	vpnFPM, base_vpnFPM;
vec3_FPM_t	vrightFPM, base_vrightFPM;
vec3_FPM_t	r_originFPM;
mvertex_FPM_t	*r_pcurrentvertbaseFPM;
btofpoly_FPM_t	*pbtofpolysFPM;
fixedpoint_t	r_aliasuvscaleFPM = FPM_FROMFLOATC(1.0);
vec3_FPM_t	viewlightvecFPM;
alight_FPM_t r_viewlightingFPM = {FPM_FROMLONGC(128), FPM_FROMLONGC(192), viewlightvecFPM};
#endif //USEFPM

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

float	dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
float	se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;

void R_MarkLeaves (void);

cvar_t	r_draworder = {"r_draworder","0"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_timegraph = {"r_timegraph","0"};
cvar_t	r_graphheight = {"r_graphheight","10"};
cvar_t	r_clearcolor = {"r_clearcolor","2"};
cvar_t	r_waterwarp = {"r_waterwarp","1"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
cvar_t	r_aliasstats = {"r_polymodelstats","0"};
cvar_t	r_dspeeds = {"r_dspeeds","0"};
cvar_t	r_drawflat = {"r_drawflat", "0"};
cvar_t	r_ambient = {"r_ambient", "0"};
cvar_t	r_reportsurfout = {"r_reportsurfout", "0"};
cvar_t	r_maxsurfs = {"r_maxsurfs", "0"};
cvar_t	r_numsurfs = {"r_numsurfs", "0"};
cvar_t	r_reportedgeout = {"r_reportedgeout", "0"};
cvar_t	r_maxedges = {"r_maxedges", "0"};
cvar_t	r_numedges = {"r_numedges", "0"};
cvar_t	r_aliastransbase = {"r_aliastransbase", "200"};
cvar_t	r_aliastransadj = {"r_aliastransadj", "100"};
//Dan East: Added:
cvar_t	r_maxparticles = {"r_maxparticles","2048"};

extern cvar_t	scr_fov;

void CreatePassages (void);
void SetVisibilityByPassages (void);

/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;
	
// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	
}

/*
===============
R_Init
===============
*/
void R_Init (void)
{
	int		dummy;
	
// get stack position so we can guess if we are going to overflow
	r_stack_start = (byte *)&dummy;
	
	R_InitTurb ();
	
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	Cvar_RegisterVariable (&r_draworder);
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_timegraph);
	Cvar_RegisterVariable (&r_graphheight);
	Cvar_RegisterVariable (&r_drawflat);
	Cvar_RegisterVariable (&r_ambient);
	Cvar_RegisterVariable (&r_clearcolor);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_aliasstats);
	Cvar_RegisterVariable (&r_dspeeds);
	Cvar_RegisterVariable (&r_reportsurfout);
	Cvar_RegisterVariable (&r_maxsurfs);
	Cvar_RegisterVariable (&r_numsurfs);
	Cvar_RegisterVariable (&r_reportedgeout);
	Cvar_RegisterVariable (&r_maxedges);
	Cvar_RegisterVariable (&r_numedges);
	Cvar_RegisterVariable (&r_aliastransbase);
	Cvar_RegisterVariable (&r_aliastransadj);
	//Dan East:
	Cvar_RegisterVariable (&r_maxparticles);

	Cvar_SetValue ("r_maxedges", (float)NUMSTACKEDGES);
	Cvar_SetValue ("r_maxsurfs", (float)NUMSTACKSURFACES);

	view_clipplanes[0].leftedge = true;
	view_clipplanes[1].rightedge = true;
	view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
			view_clipplanes[3].leftedge = false;
	view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
			view_clipplanes[3].rightedge = false;

	r_refdef.xOrigin = XCENTERING;
	r_refdef.yOrigin = YCENTERING;

	R_InitParticles ();

// TODO: collect 386-specific code in one place
#if	id386
	Sys_MakeCodeWriteable ((long)R_EdgeCodeStart,
					     (long)R_EdgeCodeEnd - (long)R_EdgeCodeStart);
#endif	// id386

	D_Init ();
}

#ifdef USEFPM
void R_InitFPM (void)
{
	int		dummy;
	
// get stack position so we can guess if we are going to overflow
	r_stack_start = (byte *)&dummy;
	
	R_InitTurb ();

//#ifndef USEFLOAT	
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	Cvar_RegisterVariable (&r_draworder);
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_timegraph);
	Cvar_RegisterVariable (&r_graphheight);
	Cvar_RegisterVariable (&r_drawflat);
	Cvar_RegisterVariable (&r_ambient);
	Cvar_RegisterVariable (&r_clearcolor);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_aliasstats);
	Cvar_RegisterVariable (&r_dspeeds);
	Cvar_RegisterVariable (&r_reportsurfout);
	Cvar_RegisterVariable (&r_maxsurfs);
	Cvar_RegisterVariable (&r_numsurfs);
	Cvar_RegisterVariable (&r_reportedgeout);
	Cvar_RegisterVariable (&r_maxedges);
	Cvar_RegisterVariable (&r_numedges);
	Cvar_RegisterVariable (&r_aliastransbase);
	Cvar_RegisterVariable (&r_aliastransadj);

	Cvar_SetValue ("r_maxedges", (float)NUMSTACKEDGES);
	Cvar_SetValue ("r_maxsurfs", (float)NUMSTACKSURFACES);
//#endif

	view_clipplanesFPM[0].leftedge = true;
	view_clipplanesFPM[1].rightedge = true;
	view_clipplanesFPM[1].leftedge = view_clipplanesFPM[2].leftedge =
			view_clipplanesFPM[3].leftedge = false;
	view_clipplanesFPM[0].rightedge = view_clipplanesFPM[2].rightedge =
			view_clipplanesFPM[3].rightedge = false;

	r_refdefFPM.xOrigin = XCENTERINGFPM;
	r_refdefFPM.yOrigin = YCENTERINGFPM;

	R_InitParticlesFPM ();

// TODO: collect 386-specific code in one place
//Dan: id386 not defined for our builds
#if	id386
	Sys_MakeCodeWriteable ((long)R_EdgeCodeStart,
					     (long)R_EdgeCodeEnd - (long)R_EdgeCodeStart);
#endif	// id386

	D_InitFPM ();
}
#endif //USEFPM

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;
	
// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;
		 	
	r_viewleaf = NULL;
	R_ClearParticles ();

	r_cnumsurfs = (int)r_maxsurfs.value;

	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;

	if (r_cnumsurfs > NUMSTACKSURFACES)
	{
		surfaces = Hunk_AllocName (r_cnumsurfs * sizeof(surf_t), "surfaces");
		surface_p = surfaces;
		surf_max = &surfaces[r_cnumsurfs];
		r_surfsonstack = false;
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
		R_SurfacePatch ();
	}
	else
	{
		r_surfsonstack = true;
	}

	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;

	r_numallocatededges = (int)r_maxedges.value;

	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;

	if (r_numallocatededges <= NUMSTACKEDGES)
	{
		auxedges = NULL;
	}
	else
	{
		auxedges = Hunk_AllocName (r_numallocatededges * sizeof(edge_t),
								   "edges");
	}

	r_dowarpold = false;
	r_viewchanged = false;
#ifdef PASSAGES
CreatePassages ();
#endif
}

#ifdef USEFPM
void R_NewMapFPM (void)
{
	int		i;
	
// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<clFPM.worldmodel->numleafs ; i++)
		clFPM.worldmodel->leafs[i].efrags = NULL;
		 	
	r_viewleafFPM = NULL;
	R_ClearParticlesFPM ();

	r_cnumsurfs = (int)r_maxsurfs.value;

	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;

	if (r_cnumsurfs > NUMSTACKSURFACES)
	{
		surfacesFPM = Hunk_AllocName (r_cnumsurfs * sizeof(surf_FPM_t), "surfaces");
		surface_FPM_p = surfacesFPM;
		surf_maxFPM = &surfacesFPM[r_cnumsurfs];
		r_surfsonstack = false;
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfacesFPM--;
		R_SurfacePatchFPM ();
	}
	else
	{
		r_surfsonstack = true;
	}

	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;

	r_numallocatededges = (int)r_maxedges.value;

	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;

	if (r_numallocatededges <= NUMSTACKEDGES)
	{
		auxedgesFPM = NULL;
	}
	else
	{
		auxedgesFPM = Hunk_AllocName (r_numallocatededges * sizeof(edge_FPM_t),
								   "edges");
	}

	r_dowarpold = false;
	r_viewchanged = false;
//Dan: not defined for our builds
#ifdef PASSAGES
CreatePassages ();
#endif
}
#endif //USEFPM

/*
===============
R_SetVrect
===============
*/
void R_SetVrect (vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
	int		h;
	float	size;

	size = scr_viewsize.value > 100 ? 100 : scr_viewsize.value;
	if (cl.intermission)
	{
		size = 100;
		lineadj = 0;
	}
	size /= 100;

	h = pvrectin->height - lineadj;
	pvrect->width = (int)(pvrectin->width * size);
	if (pvrect->width < 96)
	{
		size = (float)(96.0 / pvrectin->width);
		pvrect->width = 96;	// min for icons
	}
	pvrect->width &= ~7;
	pvrect->height = (int)(pvrectin->height * size);
	if (pvrect->height > pvrectin->height - lineadj)
		pvrect->height = pvrectin->height - lineadj;

	pvrect->height &= ~1;

	pvrect->x = (pvrectin->width - pvrect->width)/2;
	pvrect->y = (h - pvrect->height)/2;

	{
		if (lcd_x.value)
		{
			pvrect->y >>= 1;
			pvrect->height >>= 1;
		}
	}
}

#ifdef USEFPM
void R_SetVrectFPM (vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
	int		h;
	fixedpoint_t	size;
	fixedpoint_t	tmp;

	tmp=FPM_FROMFLOAT(scr_viewsize.value);
	size = tmp > FPM_FROMLONGC(100) ? FPM_FROMLONGC(100) : tmp;
	if (clFPM.intermission)
	{
		size = FPM_FROMLONGC(100);
		lineadj = 0;
	}
	size = FPM_DIVINT(size, 100);

	h = pvrectin->height - lineadj;
	pvrect->width = FPM_TOLONG(FPM_MUL(FPM_FROMLONG(pvrectin->width), size));
	if (pvrect->width < 96)
	{
		size = FPM_DIVINT(FPM_FROMFLOATC(96.0), pvrectin->width);
		pvrect->width = 96;	// min for icons
	}
	pvrect->width &= ~7;
	pvrect->height = FPM_TOLONG(FPM_DIV(FPM_FROMLONG(pvrectin->height), size));
	if (pvrect->height > pvrectin->height - lineadj)
		pvrect->height = pvrectin->height - lineadj;

	pvrect->height &= ~1;

	pvrect->x = (pvrectin->width - pvrect->width)/2;
	pvrect->y = (h - pvrect->height)/2;

	{
		if (lcd_x.value)
		{
			pvrect->y >>= 1;
			pvrect->height >>= 1;
		}
	}
}
#endif //USEFPM

/*
===============
R_ViewChanged

Called every time the vid structure or r_refdef changes.
Guaranteed to be called before the first refresh
===============
*/
#ifndef USE_PQ_OPT
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{
	int		i;
	float	res_scale;

	r_viewchanged = true;

	R_SetVrect (pvrect, &r_refdef.vrect, lineadj);

	r_refdef.horizontalFieldOfView = (float)(2.0 * tan (r_refdef.fov_x/360*M_PI));
	r_refdef.fvrectx = (float)r_refdef.vrect.x;
	r_refdef.fvrectx_adj = (float)(r_refdef.vrect.x - 0.5);
	r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x<<20) + (1<<19) - 1;
	r_refdef.fvrecty = (float)r_refdef.vrect.y;
	r_refdef.fvrecty_adj = (float)(r_refdef.vrect.y - 0.5);
	r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
	r_refdef.vrectright_adj_shift20 = (r_refdef.vrectright<<20) + (1<<19) - 1;
	r_refdef.fvrectright = (float)r_refdef.vrectright;
	r_refdef.fvrectright_adj = (float)(r_refdef.vrectright - 0.5);
	r_refdef.vrectrightedge = (float)(r_refdef.vrectright - 0.99);
	r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
	r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
	r_refdef.fvrectbottom_adj = (float)(r_refdef.vrectbottom - 0.5);

	r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
	r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
	r_refdef.aliasvrect.width = (int)(r_refdef.vrect.width * r_aliasuvscale);
	r_refdef.aliasvrect.height = (int)(r_refdef.vrect.height * r_aliasuvscale);
	r_refdef.aliasvrectright = r_refdef.aliasvrect.x +
			r_refdef.aliasvrect.width;
	r_refdef.aliasvrectbottom = r_refdef.aliasvrect.y +
			r_refdef.aliasvrect.height;

	pixelAspect = aspect;
	xOrigin = r_refdef.xOrigin;
	yOrigin = r_refdef.yOrigin;
	
	screenAspect = r_refdef.vrect.width*pixelAspect /
			r_refdef.vrect.height;
// 320*200 1.0 pixelAspect = 1.6 screenAspect
// 320*240 1.0 pixelAspect = 1.3333 screenAspect
// proper 320*200 pixelAspect = 0.8333333

	verticalFieldOfView = r_refdef.horizontalFieldOfView / screenAspect;

// values for perspective projection
// if math were exact, the values would range from 0.5 to to range+0.5
// hopefully they wll be in the 0.000001 to range+.999999 and truncate
// the polygon rasterization will never render in the first row or column
// but will definately render in the [range] row and column, so adjust the
// buffer origin to get an exact edge to edge fill
	xcenter = (float)(((float)r_refdef.vrect.width * XCENTERING) +
			r_refdef.vrect.x - 0.5);
	aliasxcenter = xcenter * r_aliasuvscale;
	ycenter = (float)(((float)r_refdef.vrect.height * YCENTERING) +
			r_refdef.vrect.y - 0.5);
	aliasycenter = ycenter * r_aliasuvscale;

	xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
	aliasxscale = xscale * r_aliasuvscale;
	xscaleinv = (float)(1.0 / xscale);
	yscale = xscale * pixelAspect;
	aliasyscale = yscale * r_aliasuvscale;
	yscaleinv = (float)(1.0 / yscale);
	xscaleshrink = (r_refdef.vrect.width-6)/r_refdef.horizontalFieldOfView;
	yscaleshrink = xscaleshrink*pixelAspect;

#ifdef USE_PQ_OPT1
	xscale_fxp=(int)(xscale*8388608.0);	//9.23
	yscale_fxp=(int)(yscale*8388608.0);	//9.23
	xcenter_fxp=(int)(xcenter*4194304.0);	//10.22
	ycenter_fxp=(int)(ycenter*4194304.0);	//10.22
	r_refdef_fvrectx_adj_fxp=(int)(r_refdef.fvrectx_adj*4194304.0);
	r_refdef_fvrectright_adj_fxp=(int)(r_refdef.fvrectright_adj*4194304.0);
	r_refdef_fvrecty_adj_fxp=(int)(r_refdef.fvrecty_adj*4194304.0);
	r_refdef_fvrectbottom_adj_fxp=(int)(r_refdef.fvrectbottom_adj*4194304.0);
#endif

// left side clip
	screenedge[0].normal[0] = (float)(-1.0 / (xOrigin*r_refdef.horizontalFieldOfView));
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ;
	
// right side clip
	screenedge[1].normal[0] =
			(float)(1.0 / ((1.0-xOrigin)*r_refdef.horizontalFieldOfView));
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ;
	
// top side clip
	screenedge[2].normal[0] = 0;
	screenedge[2].normal[1] = (float)(-1.0 / (yOrigin*verticalFieldOfView));
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ;
	
// bottom side clip
	screenedge[3].normal[0] = 0;
	screenedge[3].normal[1] = (float)(1.0 / ((1.0-yOrigin)*verticalFieldOfView));
	screenedge[3].normal[2] = 1;	
	screenedge[3].type = PLANE_ANYZ;
	
	for (i=0 ; i<4 ; i++)
		VectorNormalizeNoRet (screenedge[i].normal);

	res_scale = (float)(sqrt ((double)(r_refdef.vrect.width * r_refdef.vrect.height) /
			          (320.0 * 152.0)) *
			(2.0 / r_refdef.horizontalFieldOfView));
	r_aliastransition = r_aliastransbase.value * res_scale;
	r_resfudge = r_aliastransadj.value * res_scale;

	if (scr_fov.value <= 90.0)
		r_fov_greater_than_90 = false;
	else
		r_fov_greater_than_90 = true;

// TODO: collect 386-specific code in one place
#if	id386
	if (r_pixbytes == 1)
	{
		Sys_MakeCodeWriteable ((long)R_Surf8Start,
						     (long)R_Surf8End - (long)R_Surf8Start);
		colormap = vid.colormap;
		R_Surf8Patch ();
	}
	else
	{
		Sys_MakeCodeWriteable ((long)R_Surf16Start,
						     (long)R_Surf16End - (long)R_Surf16Start);
		colormap = vid.colormap16;
		R_Surf16Patch ();
	}
#endif	// id386

	D_ViewChanged ();
}
#else
//JB: Optimization
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{
	int		i;
	float	res_scale;

	r_viewchanged = true;

	R_SetVrect (pvrect, &r_refdef.vrect, lineadj);

	r_refdef.horizontalFieldOfView = (float)(2.0 * tan (r_refdef.fov_x/360*M_PI));
	r_refdef.fvrectx = (float)r_refdef.vrect.x;
	r_refdef.fvrectx_adj = (float)(r_refdef.vrect.x - 0.5);
	r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x<<20) + (1<<19) - 1;
	r_refdef.fvrecty = (float)r_refdef.vrect.y;
	r_refdef.fvrecty_adj = (float)(r_refdef.vrect.y - 0.5);
	r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
	r_refdef.vrectright_adj_shift20 = (r_refdef.vrectright<<20) + (1<<19) - 1;
	r_refdef.fvrectright = (float)r_refdef.vrectright;
	r_refdef.fvrectright_adj = (float)(r_refdef.vrectright - 0.5);
	r_refdef.vrectrightedge = (float)(r_refdef.vrectright - 0.99);
	r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
	r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
	r_refdef.fvrectbottom_adj = (float)(r_refdef.vrectbottom - 0.5);

	r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
	r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
	r_refdef.aliasvrect.width = (int)(r_refdef.vrect.width * r_aliasuvscale);
	r_refdef.aliasvrect.height = (int)(r_refdef.vrect.height * r_aliasuvscale);
	r_refdef.aliasvrectright = r_refdef.aliasvrect.x +
			r_refdef.aliasvrect.width;
	r_refdef.aliasvrectbottom = r_refdef.aliasvrect.y +
			r_refdef.aliasvrect.height;

	pixelAspect = aspect;
	xOrigin = r_refdef.xOrigin;
	yOrigin = r_refdef.yOrigin;
	
	screenAspect = r_refdef.vrect.width*pixelAspect /
			r_refdef.vrect.height;
// 320*200 1.0 pixelAspect = 1.6 screenAspect
// 320*240 1.0 pixelAspect = 1.3333 screenAspect
// proper 320*200 pixelAspect = 0.8333333

	verticalFieldOfView = r_refdef.horizontalFieldOfView / screenAspect;

// values for perspective projection
// if math were exact, the values would range from 0.5 to to range+0.5
// hopefully they wll be in the 0.000001 to range+.999999 and truncate
// the polygon rasterization will never render in the first row or column
// but will definately render in the [range] row and column, so adjust the
// buffer origin to get an exact edge to edge fill
	xcenter = (float)(((float)r_refdef.vrect.width * XCENTERING) +
			r_refdef.vrect.x - 0.5);
	//fpxcenter = (int)(64.0f * xcenter); // bug
	aliasxcenter = xcenter * r_aliasuvscale;
	ycenter = (float)(((float)r_refdef.vrect.height * YCENTERING) +
			r_refdef.vrect.y - 0.5);
	//fpycenter = (int)(64.0f * ycenter); // bug
	aliasycenter = ycenter * r_aliasuvscale;

	xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
	aliasxscale = xscale * r_aliasuvscale;
	xscaleinv = (float)(1.0 / xscale);
	yscale = xscale * pixelAspect;
	//fpxscale = (int)(1024.0f * xscale); // bug - FW
	//fpyscale = (int)(1024.0f * yscale); // bug - FW
	aliasyscale = yscale * r_aliasuvscale;
	yscaleinv = (float)(1.0 / yscale);
	xscaleshrink = (r_refdef.vrect.width-6)/r_refdef.horizontalFieldOfView;
	yscaleshrink = xscaleshrink*pixelAspect;

#ifdef USE_PQ_OPT1
	xscale_fxp=(int)(xscale*8388608.0);
	yscale_fxp=(int)(yscale*8388608.0);
	xcenter_fxp=(int)(xcenter*8388608.0);
	ycenter_fxp=(int)(ycenter*8388608.0);
	r_refdef_fvrectx_adj_fxp=(int)(r_refdef.fvrectx_adj*8388608.0);
	r_refdef_fvrectright_adj_fxp=(int)(r_refdef.fvrectright_adj*8388608.0);
	r_refdef_fvrecty_adj_fxp=(int)(r_refdef.fvrecty_adj*8388608.0);
	r_refdef_fvrectbottom_adj_fxp=(int)(r_refdef.fvrectbottom_adj*8388608.0);
#endif

// left side clip
	screenedge[0].normal[0] = (float)(-1.0 / (xOrigin*r_refdef.horizontalFieldOfView));
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ;
	
// right side clip
	screenedge[1].normal[0] =
			(float)(1.0 / ((1.0-xOrigin)*r_refdef.horizontalFieldOfView));
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ;
	
// top side clip
	screenedge[2].normal[0] = 0;
	screenedge[2].normal[1] = (float)(-1.0 / (yOrigin*verticalFieldOfView));
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ;
	
// bottom side clip
	screenedge[3].normal[0] = 0;
	screenedge[3].normal[1] = (float)(1.0 / ((1.0-yOrigin)*verticalFieldOfView));
	screenedge[3].normal[2] = 1;	
	screenedge[3].type = PLANE_ANYZ;
	
	for (i=0 ; i<4 ; i++)
		VectorNormalize (screenedge[i].normal);

	res_scale = (float)(sqrt ((double)(r_refdef.vrect.width * r_refdef.vrect.height) /
			          (320.0 * 152.0)) *
			(2.0 / r_refdef.horizontalFieldOfView));
	r_aliastransition = r_aliastransbase.value * res_scale;
	r_resfudge = r_aliastransadj.value * res_scale;

	if (scr_fov.value <= 90.0)
		r_fov_greater_than_90 = false;
	else
		r_fov_greater_than_90 = true;

// TODO: collect 386-specific code in one place
#if	id386
	if (r_pixbytes == 1)
	{
		Sys_MakeCodeWriteable ((long)R_Surf8Start,
						     (long)R_Surf8End - (long)R_Surf8Start);
		colormap = vid.colormap;
		R_Surf8Patch ();
	}
	else
	{
		Sys_MakeCodeWriteable ((long)R_Surf16Start,
						     (long)R_Surf16End - (long)R_Surf16Start);
		colormap = vid.colormap16;
		R_Surf16Patch ();
	}
#endif	// id386

	D_ViewChanged ();
}
#endif
/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;

	if (r_oldviewleaf == r_viewleaf)
		return;
	
	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);
		
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

#ifdef USEFPM
void R_MarkLeavesFPM (void)
{
	byte		*vis;
	mnode_FPM_t	*node;
	int			i;

	if (r_oldviewleafFPM == r_viewleafFPM)
		return;
	
	r_visframecount++;
	r_oldviewleafFPM = r_viewleafFPM;

	vis = Mod_LeafPVSFPM (r_viewleafFPM, clFPM.worldmodel);
		
	for (i=0 ; i<clFPM.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_FPM_t *)&clFPM.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}
#endif //USEFPM

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int			i, j;
	int			lnum;
	alight_t	lighting;
// FIXME: remove and do real lighting
	float		lightvec[3] = {-1, 0, 0};
	vec3_t		dist;
	float		add;

	if (!r_drawentities.value)
		return;

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		if (currententity == &cl_entities[cl.viewentity])
			continue;	// don't draw the player

		switch (currententity->model->type)
		{
		case mod_sprite:
			VectorCopy (currententity->origin, r_entorigin);
			VectorSubtract (r_origin, r_entorigin, modelorg);
			R_DrawSprite ();
			break;

		case mod_alias:
			VectorCopy (currententity->origin, r_entorigin);
			VectorSubtract (r_origin, r_entorigin, modelorg);

		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status
			if (R_AliasCheckBBox ())
			{
				j = R_LightPoint (currententity->origin);
	
				lighting.ambientlight = j;
				lighting.shadelight = j;

				lighting.plightvec = lightvec;

				for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
				{
					if (cl_dlights[lnum].die >= cl.time)
					{
						VectorSubtract (currententity->origin,
										cl_dlights[lnum].origin,
										dist);
						add = cl_dlights[lnum].radius - Length(dist);
	
						if (add > 0)
							lighting.ambientlight += (int)add;
					}
				}
	
			// clamp lighting so it doesn't overbright as much
				if (lighting.ambientlight > 128)
					lighting.ambientlight = 128;
				if (lighting.ambientlight + lighting.shadelight > 192)
					lighting.shadelight = 192 - lighting.ambientlight;

				R_AliasDrawModel (&lighting);
			}

			break;

		default:
			break;
		}
	}
}

#ifdef USEFPM
void R_DrawEntitiesOnListFPM (void)
{
	int				i, j;
	int				lnum;
	alight_FPM_t	lighting;
// FIXME: remove and do real lighting (not Dan's comment)
	fixedpoint_t	lightvec[3] = {FPM_FROMLONGC(-1), 0, 0};
	vec3_FPM_t		dist;
	fixedpoint_t	add;

	if (!r_drawentities.value)
		return;

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententityFPM = cl_visedictsFPM[i];

		if (currententityFPM == &cl_entitiesFPM[clFPM.viewentity])
			continue;	// don't draw the player

		switch (currententityFPM->model->type)
		{
		case mod_sprite:
			VectorCopy (currententityFPM->origin, r_entoriginFPM);
			VectorSubtractFPM (r_originFPM, r_entoriginFPM, modelorgFPM);
			R_DrawSpriteFPM ();
			break;

		case mod_alias:
			VectorCopy (currententityFPM->origin, r_entoriginFPM);
			VectorSubtractFPM (r_originFPM, r_entoriginFPM, modelorgFPM);

		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status
			if (R_AliasCheckBBoxFPM ())
			{
				j = R_LightPointFPM (currententityFPM->origin);
	
				lighting.ambientlight = j;
				lighting.shadelight = j;

				lighting.plightvec = lightvec;

				for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
				{
					if (cl_dlightsFPM[lnum].die >= cl.time)
					{
						VectorSubtractFPM (currententityFPM->origin,
										cl_dlightsFPM[lnum].origin,
										dist);
						add = cl_dlightsFPM[lnum].radius - LengthFPM(dist);
	
						if (add > 0)
							lighting.ambientlight += (int)add;
					}
				}
	
			// clamp lighting so it doesn't overbright as much
				if (lighting.ambientlight > 128)
					lighting.ambientlight = 128;
				if (lighting.ambientlight + lighting.shadelight > 192)
					lighting.shadelight = 192 - lighting.ambientlight;

				R_AliasDrawModelFPM (&lighting);
			}

			break;

		default:
			break;
		}
	}
}
#endif //USEFPM

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
// FIXME: remove and do real lighting
	float		lightvec[3] = {-1, 0, 0};
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	
	if (!r_drawviewmodel.value || r_fov_greater_than_90)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;

	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	VectorCopy (vup, viewlightvec);
	VectorInverse (viewlightvec);

	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	r_viewlighting.ambientlight = j;
	r_viewlighting.shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - Length(dist);
		if (add > 0)
			r_viewlighting.ambientlight += (int)add;
	}

// clamp lighting so it doesn't overbright as much
	if (r_viewlighting.ambientlight > 128)
		r_viewlighting.ambientlight = 128;
	if (r_viewlighting.ambientlight + r_viewlighting.shadelight > 192)
		r_viewlighting.shadelight = 192 - r_viewlighting.ambientlight;

	r_viewlighting.plightvec = lightvec;

#ifdef QUAKE2
	cl.light_level = r_viewlighting.ambientlight;
#endif

	R_AliasDrawModel (&r_viewlighting);
}

#ifdef USEFPM
void R_DrawViewModelFPM (void)
{
// FIXME: remove and do real lighting
	fixedpoint_t	lightvec[3] = {FPM_FROMLONG(-1), 0, 0};
	int				j;
	int				lnum;
	vec3_FPM_t		dist;
	fixedpoint_t	add;
	dlight_FPM_t	*dl;
	
	if (!r_drawviewmodel.value || r_fov_greater_than_90)
		return;

	if (clFPM.items & IT_INVISIBILITY)
		return;

	if (clFPM.stats[STAT_HEALTH] <= 0)
		return;

	currententityFPM = &clFPM.viewent;
	if (!currententityFPM->model)
		return;

	VectorCopy (currententityFPM->origin, r_entoriginFPM);
	VectorSubtractFPM (r_originFPM, r_entoriginFPM, modelorgFPM);

	VectorCopy (vupFPM, viewlightvecFPM);
	VectorInverseFPM (viewlightvecFPM);

	j = R_LightPointFPM (currententityFPM->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	r_viewlightingFPM.ambientlight = j;
	r_viewlightingFPM.shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlightsFPM[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtractFPM (currententityFPM->origin, dl->origin, dist);
		add = dl->radius - LengthFPM(dist);
		if (add > 0)
			r_viewlightingFPM.ambientlight += (int)add;
	}

// clamp lighting so it doesn't overbright as much
	if (r_viewlightingFPM.ambientlight > 128)
		r_viewlightingFPM.ambientlight = 128;
	if (r_viewlightingFPM.ambientlight + r_viewlightingFPM.shadelight > 192)
		r_viewlightingFPM.shadelight = 192 - r_viewlightingFPM.ambientlight;

	r_viewlightingFPM.plightvec = lightvec;

#ifdef QUAKE2
	clFPM.light_level = r_viewlightingFPM.ambientlight;
#endif

	R_AliasDrawModelFPM (&r_viewlightingFPM);
}
#endif //USEFPM

/*
=============
R_BmodelCheckBBox
=============
*/
int R_BmodelCheckBBox (model_t *clmodel, float *minmaxs)
{
	int			i, *pindex, clipflags;
	vec3_t		acceptpt, rejectpt;
	double		d;

	clipflags = 0;

	if (currententity->angles[0] || currententity->angles[1]
		|| currententity->angles[2])
	{
		for (i=0 ; i<4 ; i++)
		{
			d = DotProduct (currententity->origin, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= -clmodel->radius)
				return BMODEL_FULLY_CLIPPED;

			if (d <= clmodel->radius)
				clipflags |= (1<<i);
		}
	}
	else
	{
		for (i=0 ; i<4 ; i++)
		{
		// generate accept and reject points
		// FIXME: do with fast look-ups or integer tests based on the sign bit
		// of the floating point values

			pindex = pfrustum_indexes[i];

			rejectpt[0] = minmaxs[pindex[0]];
			rejectpt[1] = minmaxs[pindex[1]];
			rejectpt[2] = minmaxs[pindex[2]];
			
			d = DotProduct (rejectpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				return BMODEL_FULLY_CLIPPED;

			acceptpt[0] = minmaxs[pindex[3+0]];
			acceptpt[1] = minmaxs[pindex[3+1]];
			acceptpt[2] = minmaxs[pindex[3+2]];

			d = DotProduct (acceptpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				clipflags |= (1<<i);
		}
	}

	return clipflags;
}

#ifdef USEFPM
int R_BmodelCheckBBoxFPM (model_FPM_t *clmodel, fixedpoint_t *minmaxs)
{
	int				i, *pindex, clipflags;
	vec3_FPM_t		acceptpt, rejectpt;
	fixedpoint_t	d;	//Dan: potential overflow / underflow (was a double)

	clipflags = 0;

	if (currententityFPM->angles[0] || currententityFPM->angles[1]
		|| currententityFPM->angles[2])
	{
		for (i=0 ; i<4 ; i++)
		{
			d = DotProductFPM (currententityFPM->origin, view_clipplanesFPM[i].normal);
			d = FPM_SUB(d, view_clipplanesFPM[i].dist);

			if (d <= -clmodel->radius)
				return BMODEL_FULLY_CLIPPED;

			if (d <= clmodel->radius)
				clipflags |= (1<<i);
		}
	}
	else
	{
		for (i=0 ; i<4 ; i++)
		{
			long long accum, mul;
		// generate accept and reject points
		// FIXME: do with fast look-ups or integer tests based on the sign bit
		// of the floating point values

			pindex = pfrustum_indexes[i];

			rejectpt[0] = minmaxs[pindex[0]];
			rejectpt[1] = minmaxs[pindex[1]];
			rejectpt[2] = minmaxs[pindex[2]];
			
			//Dan: Overflow / underflow problem with 16.16 fxp.  This calculation
			//is only used to make a logic decision, so we'll use 32.32 code.
			//This code includes Dot Product.
			accum=rejectpt[0];
			accum*=view_clipplanesFPM[i].normal[0];
			mul=rejectpt[1];
			mul*=view_clipplanesFPM[i].normal[1];
			accum+=mul;
			mul=rejectpt[2];
			mul*=view_clipplanesFPM[i].normal[2];
			accum+=mul;
			mul=view_clipplanesFPM[i].dist;
			mul<<=16;	//16.16 -> 32.32
			accum-=mul;
			if (accum<=0)
				return BMODEL_FULLY_CLIPPED;
			
			//d = DotProductFPM (rejectpt, view_clipplanesFPM[i].normal);
			//d = FPM_SUB(d, view_clipplanesFPM[i].dist);

			//if (d <= 0)
			//	return BMODEL_FULLY_CLIPPED;

			acceptpt[0] = minmaxs[pindex[3+0]];
			acceptpt[1] = minmaxs[pindex[3+1]];
			acceptpt[2] = minmaxs[pindex[3+2]];

			accum=acceptpt[0];
			accum*=view_clipplanesFPM[i].normal[0];
			mul=acceptpt[1];
			mul*=view_clipplanesFPM[i].normal[1];
			accum+=mul;
			mul=acceptpt[2];
			mul*=view_clipplanesFPM[i].normal[2];
			accum+=mul;
			mul=view_clipplanesFPM[i].dist;
			mul<<=16;	//16.16 -> 32.32
			accum-=mul;
			if (accum <= 0)
				clipflags |= (1<<i);

			//d = DotProductFPM (acceptpt, view_clipplanesFPM[i].normal);
			//d = FPM_SUB(d, view_clipplanesFPM[i].dist);

			//if (d <= 0)
			//	clipflags |= (1<<i);
		}
	}

	return clipflags;
}
#endif //USEFPM

/*
=============
R_DrawBEntitiesOnList
=============
*/
void R_DrawBEntitiesOnList (void)
{
	int			i, j, k, clipflags;
	vec3_t		oldorigin;
	model_t		*clmodel;
	float		minmaxs[6];

	if (!r_drawentities.value)
		return;

	VectorCopy (modelorg, oldorigin);
	insubmodel = true;
	r_dlightframecount = r_framecount;

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_brush:

			clmodel = currententity->model;

		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status
			for (j=0 ; j<3 ; j++)
			{
				minmaxs[j] = currententity->origin[j] +
						clmodel->mins[j];
				minmaxs[3+j] = currententity->origin[j] +
						clmodel->maxs[j];
			}

			clipflags = R_BmodelCheckBBox (clmodel, minmaxs);

			if (clipflags != BMODEL_FULLY_CLIPPED)
			{
				VectorCopy (currententity->origin, r_entorigin);
				VectorSubtract (r_origin, r_entorigin, modelorg);
			// FIXME: is this needed?
				VectorCopy (modelorg, r_worldmodelorg);
		
				r_pcurrentvertbase = clmodel->vertexes;
                                
#ifdef USE_PQ_OPT2
				r_pcurrentvertbase_fxp = clmodel->vertexes_fxp;
#endif
		
			// FIXME: stop transforming twice
				R_RotateBmodel ();

#ifdef USE_PQ_OPT1
				modelorg_fxp[0]=(int)(modelorg[0]*524288.0);
				modelorg_fxp[1]=(int)(modelorg[1]*524288.0);
				modelorg_fxp[2]=(int)(modelorg[2]*524288.0);

				vright_fxp[0]=(int)(256.0/vright[0]);
				if (!vright_fxp[0]) vright_fxp[0]=0x7fffffff;
				vright_fxp[1]=(int)(256.0/vright[1]);
				if (!vright_fxp[1]) vright_fxp[1]=0x7fffffff;
				vright_fxp[2]=(int)(256.0/vright[2]);
				if (!vright_fxp[2]) vright_fxp[2]=0x7fffffff;

				vpn_fxp[0]=(int)(256.0/vpn[0]);
				if (!vpn_fxp[0]) vpn_fxp[0]=0x7fffffff;
				vpn_fxp[1]=(int)(256.0/vpn[1]);
				if (!vpn_fxp[1]) vpn_fxp[1]=0x7fffffff;
				vpn_fxp[2]=(int)(256.0/vpn[2]);
				if (!vpn_fxp[2]) vpn_fxp[2]=0x7fffffff;

				vup_fxp[0]=(int)(256.0/vup[0]);
				if (!vup_fxp[0]) vup_fxp[0]=0x7fffffff;
				vup_fxp[1]=(int)(256.0/vup[1]);
				if (!vup_fxp[1]) vup_fxp[1]=0x7fffffff;
				vup_fxp[2]=(int)(256.0/vup[2]);
				if (!vup_fxp[2]) vup_fxp[2]=0x7fffffff;
#endif
                                
			// calculate dynamic lighting for bmodel if it's not an
			// instanced model
				if (clmodel->firstmodelsurface != 0)
				{
					for (k=0 ; k<MAX_DLIGHTS ; k++)
					{
						if ((cl_dlights[k].die < cl.time) ||
							(!cl_dlights[k].radius))
						{
							continue;
						}

						R_MarkLights (&cl_dlights[k], 1<<k,
							clmodel->nodes + clmodel->hulls[0].firstclipnode);
					}
				}

			// if the driver wants polygons, deliver those. Z-buffering is on
			// at this point, so no clipping to the world tree is needed, just
			// frustum clipping
				if (r_drawpolys | r_drawculledpolys)
				{
					R_ZDrawSubmodelPolys (clmodel);
				}
				else
				{
					r_pefragtopnode = NULL;

					for (j=0 ; j<3 ; j++)
					{
						r_emins[j] = minmaxs[j];
						r_emaxs[j] = minmaxs[3+j];
					}

					R_SplitEntityOnNode2 (cl.worldmodel->nodes);

					if (r_pefragtopnode)
					{
						currententity->topnode = r_pefragtopnode;
	
						if (r_pefragtopnode->contents >= 0)
						{
						// not a leaf; has to be clipped to the world BSP
							r_clipflags = clipflags;
							R_DrawSolidClippedSubmodelPolygons (clmodel);
						}
						else
						{
						// falls entirely in one leaf, so we just put all the
						// edges in the edge list and let 1/z sorting handle
						// drawing order
							R_DrawSubmodelPolygons (clmodel, clipflags);
						}
	
						currententity->topnode = NULL;
					}
				}

			// put back world rotation and frustum clipping		
			// FIXME: R_RotateBmodel should just work off base_vxx
				VectorCopy (base_vpn, vpn);
				VectorCopy (base_vup, vup);
				VectorCopy (base_vright, vright);
				VectorCopy (base_modelorg, modelorg);
				VectorCopy (oldorigin, modelorg);
				R_TransformFrustum ();
			}

			break;

		default:
			break;
		}
	}

	insubmodel = false;
}


/*
================
R_EdgeDrawing
================
*/
void R_EdgeDrawing (void)
{
	edge_t	ledges[NUMSTACKEDGES +
				((CACHE_SIZE - 1) / sizeof(edge_t)) + 1];
	surf_t	lsurfs[NUMSTACKSURFACES +
				((CACHE_SIZE - 1) / sizeof(surf_t)) + 1];

	if (auxedges)
	{
		r_edges = auxedges;
	}
	else
	{
		r_edges =  (edge_t *)
				(((long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	}

	if (r_surfsonstack)
	{
		surfaces =  (surf_t *)
				(((long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
		surf_max = &surfaces[r_cnumsurfs];
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
		R_SurfacePatch ();
	}

	R_BeginEdgeFrame ();

	if (r_dspeeds.value)
	{
		rw_time1 = (float)Sys_FloatTime ();
	}

	R_RenderWorld ();

	if (r_drawculledpolys)
		R_ScanEdges ();

// only the world can be drawn back to front with no z reads or compares, just
// z writes, so have the driver turn z compares on now
	D_TurnZOn ();

	if (r_dspeeds.value)
	{
		rw_time2 = (float)Sys_FloatTime ();
		db_time1 = rw_time2;
	}

	R_DrawBEntitiesOnList ();

	if (r_dspeeds.value)
	{
		db_time2 = (float)Sys_FloatTime ();
		se_time1 = db_time2;
	}

	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	if (!(r_drawpolys | r_drawculledpolys))
		R_ScanEdges ();
}

#ifdef USEFPM
void R_EdgeDrawingFPM (void)
{
	edge_FPM_t	ledges[NUMSTACKEDGES +
					((CACHE_SIZE - 1) / sizeof(edge_FPM_t)) + 1];
	surf_FPM_t	lsurfs[NUMSTACKSURFACES +
					((CACHE_SIZE - 1) / sizeof(surf_FPM_t)) + 1];
//	int i=NUMSTACKEDGES +
//				((CACHE_SIZE - 1) / sizeof(edge_t)) + 1;

	if (auxedgesFPM)
	{
		r_edgesFPM = auxedgesFPM;
	}
	else
	{
		r_edgesFPM =  (edge_FPM_t *)
				(((long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	}
	
	if (r_surfsonstack)
	{
		surfacesFPM =  (surf_FPM_t *)
				(((long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
		surf_maxFPM = &surfacesFPM[r_cnumsurfs];
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfacesFPM--;
		R_SurfacePatchFPM ();
	}
	
	R_BeginEdgeFrameFPM ();
	
	if (r_dspeeds.value)
	{
		rw_time1 = (float)Sys_FloatTime ();
	}

	R_RenderWorldFPM ();
	
	if (r_drawculledpolys)
		R_ScanEdgesFPM ();

// only the world can be drawn back to front with no z reads or compares, just
// z writes, so have the driver turn z compares on now
	//D_TurnZOn ();	//Dan: empty func
	
	if (r_dspeeds.value)
	{
		rw_time2 = (float)Sys_FloatTime ();
		db_time1 = rw_time2;
	}

	R_DrawBEntitiesOnListFPM ();

	if (r_dspeeds.value)
	{
		db_time2 = (float)Sys_FloatTime ();
		se_time1 = db_time2;
	}

	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	if (!(r_drawpolys | r_drawculledpolys))
		R_ScanEdgesFPM ();
	
}
#endif //USEFPM

extern int r_numparticles, r_allocatedparticles;
/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView_ (void)
{
	//Dan:
	byte	warpbuffer[WARP_WIDTH * WARP_HEIGHT];

	//Dan East: Here we see if the new r_maxparticles var has changed,
	//if so we change the max value. I guess this is as good a place
	//as any to perform this check.
	if (r_numparticles != (int)r_maxparticles.value) {
		r_numparticles = (int)r_maxparticles.value;
		//Check to see if we have enough particles allocated
		if (r_numparticles>=r_allocatedparticles) 
			//We don't (they've exceeded MAX_PARTICLES or the original -particles value)
			r_numparticles=r_allocatedparticles;
		R_ClearParticles();
	}

	r_warpbuffer = warpbuffer;

	if (r_timegraph.value || r_speeds.value || r_dspeeds.value)
		r_time1 = (float)Sys_FloatTime ();

	R_SetupFrame ();

#ifdef PASSAGES
SetVisibilityByPassages ();
#else
	R_MarkLeaves ();	// done here so we know if we're in water
#endif

// make FDIV fast. This reduces timing precision after we've been running for a
// while, so we don't do it globally.  This also sets chop mode, and we do it
// here so that setup stuff like the refresh area calculations match what's
// done in screen.c
	Sys_LowFPPrecision ();

	if (!cl_entities[0].model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");
		
	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	R_EdgeDrawing ();

        rb->yield(); // let sound run
        
	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	if (r_dspeeds.value)
	{
		se_time2 = (float)Sys_FloatTime ();
		de_time1 = se_time2;
	}

	R_DrawEntitiesOnList ();

	if (r_dspeeds.value)
	{
		de_time2 = (float)Sys_FloatTime ();
		dv_time1 = de_time2;
	}

	R_DrawViewModel ();

	if (r_dspeeds.value)
	{
		dv_time2 = (float)Sys_FloatTime ();
		dp_time1 = (float)Sys_FloatTime ();
	}

	R_DrawParticles ();

        rb->yield(); // let sound run

	if (r_dspeeds.value)
		dp_time2 = (float)Sys_FloatTime ();

	if (r_dowarp)
		D_WarpScreen ();

	V_SetContentsColor (r_viewleaf->contents);

	if (r_timegraph.value)
		R_TimeGraph ();

	if (r_aliasstats.value)
		R_PrintAliasStats ();
		
	if (r_speeds.value)
		R_PrintTimes ();

	if (r_dspeeds.value)
		R_PrintDSpeeds ();

	if (r_reportsurfout.value && r_outofsurfaces)
		Con_Printf ("Short %d surfaces\n", r_outofsurfaces);

	if (r_reportedgeout.value && r_outofedges)
		Con_Printf ("Short roughly %d edges\n", r_outofedges * 2 / 3);

// back to high floating-point precision
	Sys_HighFPPrecision ();
}

#ifdef USEFPM
void R_RenderViewFPM_ (void)
{
	//Dan:
//	byte	*warpbuffer=malloc(WARP_WIDTH * WARP_HEIGHT);
	byte	warpbuffer[WARP_WIDTH * WARP_HEIGHT];

	r_warpbuffer = warpbuffer;
	
	if (r_timegraph.value || r_speeds.value || r_dspeeds.value)
		r_time1 = (float)Sys_FloatTime ();
	
	R_SetupFrameFPM ();
	
#ifdef PASSAGES
	SetVisibilityByPassages ();
#else
	R_MarkLeavesFPM ();	// done here so we know if we're in water
#endif
	
// make FDIV fast. This reduces timing precision after we've been running for a
// while, so we don't do it globally.  This also sets chop mode, and we do it
// here so that setup stuff like the refresh area calculations match what's
// done in screen.c
	Sys_LowFPPrecision ();
	
	if (!cl_entitiesFPM[0].model || !clFPM.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");
	
	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	R_EdgeDrawingFPM ();
	
	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	if (r_dspeeds.value)
	{
		se_time2 = (float)Sys_FloatTime ();
		de_time1 = se_time2;
	}

	R_DrawEntitiesOnListFPM ();
	
	if (r_dspeeds.value)
	{
		de_time2 = (float)Sys_FloatTime ();
		dv_time1 = de_time2;
	}

	R_DrawViewModelFPM ();
	
	if (r_dspeeds.value)
	{
		dv_time2 = (float)Sys_FloatTime ();
		dp_time1 = (float)Sys_FloatTime ();
	}

	R_DrawParticlesFPM ();
	
	if (r_dspeeds.value)
		dp_time2 = (float)Sys_FloatTime ();

	if (r_dowarp)
		D_WarpScreen ();
	
	V_SetContentsColorFPM (r_viewleafFPM->contents);

	if (r_timegraph.value)
		R_TimeGraphFPM ();

	if (r_aliasstats.value)
		R_PrintAliasStats ();
		
	if (r_speeds.value)
		R_PrintTimes ();

	if (r_dspeeds.value)
		R_PrintDSpeeds ();

	if (r_reportsurfout.value && r_outofsurfaces)
		Con_Printf ("Short %d surfaces\n", r_outofsurfaces);

	if (r_reportedgeout.value && r_outofedges)
		Con_Printf ("Short roughly %d edges\n", r_outofedges * 2 / 3);

// back to high floating-point precision
	Sys_HighFPPrecision ();
}
#endif //USEFPM

void R_RenderView (void)
{
	int		dummy;
	int		delta;
	
	delta = (byte *)&dummy - r_stack_start;
	if (delta < -10000 || delta > 10000)
		Sys_Error ("R_RenderView: called without enough stack");

	if ( Hunk_LowMark() & 3 )
		Sys_Error ("Hunk is missaligned");

	if ( (long)(&dummy) & 3 )
		Sys_Error ("Stack is missaligned");

	if ( (long)(&r_warpbuffer) & 3 )
		Sys_Error ("Globals are missaligned");

	R_RenderView_ ();
}

#ifdef USEFPM
void R_RenderViewFPM (void)
{
	int		dummy;
	int		delta;
	
	delta = (byte *)&dummy - r_stack_start;
	if (delta < -10000 || delta > 10000)
		Sys_Error ("R_RenderView: called without enough stack");

	if ( Hunk_LowMark() & 3 )
		Sys_Error ("Hunk is missaligned");

	if ( (long)(&dummy) & 3 )
		Sys_Error ("Stack is missaligned");

	if ( (long)(&r_warpbuffer) & 3 )
		Sys_Error ("Globals are missaligned");

	R_RenderViewFPM_ ();
}
#endif //USEFPM

/*
================
R_InitTurb
================
*/
void R_InitTurb (void)
{
	int		i;
	
	for (i=0 ; i<(SIN_BUFFER_SIZE) ; i++)
	{
		sintable[i] = (int)(AMP + sin(i*3.14159*2/CYCLE)*AMP);
		intsintable[i] = (int)(AMP2 + sin(i*3.14159*2/CYCLE)*AMP2);	// AMP2, not 20
	}
}

