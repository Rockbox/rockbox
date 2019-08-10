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
// r_local.h -- private refresh defs

#ifndef GLQUAKE
#include "r_shared.h"

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle

#define BMODEL_FULLY_CLIPPED	0x10 // value returned by R_BmodelCheckBBox ()
									 //  if bbox is trivially rejected

//===========================================================================
// viewmodel lighting

typedef struct {
	int			ambientlight;
	int			shadelight;
	float		*plightvec;
} alight_t;

typedef struct {
	int				ambientlight;
	int				shadelight;
	fixedpoint_t	*plightvec;
} alight_FPM_t;

//===========================================================================
// clipped bmodel edges

typedef struct bedge_s
{
	mvertex_t		*v[2];
	struct bedge_s	*pnext;
} bedge_t;

typedef struct bedge_FPM_s
{
	mvertex_FPM_t		*v[2];
	struct bedge_FPM_s	*pnext;
} bedge_FPM_t;

typedef struct {
	float	fv[3];		// viewspace x, y
} auxvert_t;

typedef struct {
	fixedpoint_t	fv[3];		// viewspace x, y
} auxvert_FPM_t;

//===========================================================================

extern cvar_t	r_draworder;
extern cvar_t	r_speeds;
extern cvar_t	r_timegraph;
extern cvar_t	r_graphheight;
extern cvar_t	r_clearcolor;
extern cvar_t	r_waterwarp;
extern cvar_t	r_fullbright;
extern cvar_t	r_drawentities;
extern cvar_t	r_aliasstats;
extern cvar_t	r_dspeeds;
extern cvar_t	r_drawflat;
extern cvar_t	r_ambient;
extern cvar_t	r_reportsurfout;
extern cvar_t	r_maxsurfs;
extern cvar_t	r_numsurfs;
extern cvar_t	r_reportedgeout;
extern cvar_t	r_maxedges;
extern cvar_t	r_numedges;
//Dan East:
extern cvar_t	r_maxparticles;

#define XCENTERING	(1.0 / 2.0)
#define YCENTERING	(1.0 / 2.0)

#define XCENTERINGFPM	FPM_FROMFLOAT(1.0 / 2.0)
#define YCENTERINGFPM	FPM_FROMFLOAT(1.0 / 2.0)

#define CLIP_EPSILON		0.001

#define BACKFACE_EPSILON	0.01

//===========================================================================

#define	DIST_NOT_SET	98765

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct clipplane_s
{
	vec3_t		normal;
	float		dist;
	struct		clipplane_s	*next;
	byte		leftedge;
	byte		rightedge;
	byte		reserved[2];
} clipplane_t;

typedef struct clipplane_fxp_s
{
	int			normal[3];
	int			dist;
	struct		clipplane_fxp_s	*next;
	byte		leftedge;
	byte		rightedge;
	byte		reserved[2];
} clipplane_fxp_t;

typedef struct clipplane_FPM_s
{
	vec3_FPM_t	normal;
	fixedpoint_t	dist;
	struct		clipplane_FPM_s	*next;
	byte		leftedge;
	byte		rightedge;
	byte		reserved[2];
} clipplane_FPM_t;

extern	clipplane_t	view_clipplanes[4];
extern	clipplane_FPM_t	view_clipplanesFPM[4];

#ifdef USE_PQ_OPT2
extern	clipplane_fxp_t	view_clipplanes_fxp[4];
#endif


//=============================================================================

void R_RenderWorld (void);
void R_RenderWorldFPM (void);

//=============================================================================

extern	mplane_t	screenedge[4];
extern	mplane_FPM_t	screenedgeFPM[4];

extern	vec3_t		r_origin;

extern	vec3_t		r_entorigin;
extern	vec3_FPM_t	r_entoriginFPM;

extern	float		screenAspect;
extern	float		verticalFieldOfView;
extern	float		xOrigin, yOrigin;

extern	int			r_visframecount;

//=============================================================================

extern int	vstartscan;


void R_ClearPolyList (void);
void R_DrawPolyList (void);

//
// current entity info
//
extern	qboolean		insubmodel;
extern	vec3_t			r_worldmodelorg;
extern	vec3_FPM_t		r_worldmodelorgFPM;

void R_DrawSprite (void);
void R_DrawSpriteFPM (void);
void R_RenderFace (msurface_t *fa, int clipflags);
void R_RenderFaceFPM (msurface_FPM_t *fa, int clipflags);
void R_RenderPoly (msurface_t *fa, int clipflags);
void R_RenderPolyFPM (msurface_FPM_t *fa, int clipflags);
void R_RenderBmodelFace (bedge_t *pedges, msurface_t *psurf);
void R_RenderBmodelFaceFPM (bedge_FPM_t *pedges, msurface_FPM_t *psurf);
void R_TransformPlane (mplane_t *p, float *normal, float *dist);
void R_TransformFrustum (void);
void R_TransformFrustumFPM (void);
void R_SetSkyFrame (void);
void R_SetSkyFrameFPM (void);
void R_DrawSurfaceBlock16 (void);
void R_DrawSurfaceBlock8 (void);
texture_t *R_TextureAnimation (texture_t *base);
texture_t *R_TextureAnimationFPM (texture_t *base);

#if	id386

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

#endif

void R_GenSkyTile (void *pdest);
void R_GenSkyTile16 (void *pdest);
void R_Surf8Patch (void);
void R_Surf16Patch (void);
void R_DrawSubmodelPolygons (model_t *pmodel, int clipflags);
void R_DrawSubmodelPolygonsFPM (model_FPM_t *pmodel, int clipflags);
void R_DrawSolidClippedSubmodelPolygons (model_t *pmodel);
void R_DrawSolidClippedSubmodelPolygonsFPM (model_FPM_t *pmodel);

void R_AddPolygonEdges (emitpoint_t *pverts, int numverts, int miplevel);
surf_t *R_GetSurf (void);
void R_AliasDrawModel (alight_t *plighting);
void R_AliasDrawModelFPM (alight_FPM_t *plighting);
void R_BeginEdgeFrame (void);
void R_BeginEdgeFrameFPM (void);
void R_ScanEdges (void);
void R_ScanEdgesFPM (void);
void D_DrawSurfaces (void);
void D_DrawSurfacesFPM (void);
void R_InsertNewEdges (edge_t *edgestoadd, edge_t *edgelist);
void R_StepActiveU (edge_t *pedge);
void R_RemoveEdges (edge_t *pedge);

extern void R_Surf8Start (void);
extern void R_Surf8End (void);
extern void R_Surf16Start (void);
extern void R_Surf16End (void);
extern void R_EdgeCodeStart (void);
extern void R_EdgeCodeEnd (void);

extern void R_RotateBmodel (void);
extern void R_RotateBmodelFPM (void);

extern int	c_faceclip;
extern int	r_polycount;
extern int	r_wholepolycount;

extern	model_t		*cl_worldmodel;

extern int		*pfrustum_indexes[4];

// !!! if this is changed, it must be changed in asm_draw.h too !!!
#define	NEAR_CLIP	0.01
#define	NEAR_CLIP_FPM	FPM_FROMFLOAT(0.01)

extern int			ubasestep, errorterm, erroradjustup, erroradjustdown;
extern int			vstartscan;

extern fixed16_t	sadjust, tadjust;
extern fixed16_t	bbextents, bbextentt;

#define MAXBVERTINDEXES	1000	// new clipped vertices when clipping bmodels
								//  to the world BSP
extern mvertex_t	*r_ptverts, *r_ptvertsmax;

extern vec3_t			sbaseaxis[3], tbaseaxis[3];
extern float			entity_rotation[3][3];

extern int		reinit_surfcache;

extern int		r_currentkey;
extern int		r_currentbkey;

typedef struct btofpoly_s {
	int			clipflags;
	msurface_t	*psurf;
} btofpoly_t;

typedef struct btofpoly_FPM_s {
	int			clipflags;
	msurface_FPM_t	*psurf;
} btofpoly_FPM_t;

#define MAX_BTOFPOLYS	5000	// FIXME: tune this

extern int			numbtofpolys;
extern btofpoly_t	*pbtofpolys;
extern btofpoly_FPM_t	*pbtofpolysFPM;

void	R_InitTurb (void);
void	R_ZDrawSubmodelPolys (model_t *clmodel);
void	R_ZDrawSubmodelPolysFPM (model_FPM_t *clmodel);

//=========================================================
// Alias models
//=========================================================

#define MAXALIASVERTS		2000	// TODO: tune this
#define ALIAS_Z_CLIP_PLANE	5
#define ALIAS_Z_CLIP_PLANE_FPM	FPM_FROMLONG(5)

extern int				numverts;
extern int				a_skinwidth;
extern mtriangle_t		*ptriangles;
extern int				numtriangles;
extern aliashdr_t		*paliashdr;
extern mdl_t			*pmdl;
extern float			leftclip, topclip, rightclip, bottomclip;
extern int				r_acliptype;
extern finalvert_t		*pfinalverts;
extern auxvert_t		*pauxverts;
extern auxvert_FPM_t	*pauxvertsFPM;

qboolean R_AliasCheckBBox (void);
qboolean R_AliasCheckBBoxFPM (void);

//=========================================================
// turbulence stuff

#define	AMP		8*0x10000
#define	AMP2	3
#define	SPEED	20

//=========================================================
// particle stuff

void R_DrawParticles (void);
void R_DrawParticlesFPM (void);
void R_InitParticles (void);
void R_InitParticlesFPM (void);
void R_ClearParticles (void);
void R_ClearParticlesFPM (void);
void R_ReadPointFile_f (void);
void R_SurfacePatch (void);
void R_SurfacePatchFPM (void);

extern int			r_amodels_drawn;
extern edge_t		*auxedges;
extern edge_FPM_t	*auxedgesFPM;
extern int			r_numallocatededges;
extern edge_t		*r_edges, *edge_p, *edge_max;
extern edge_FPM_t	*r_edgesFPM, *edge_FPM_p, *edge_maxFPM;

extern	edge_t	*newedges[MAXHEIGHT];
extern	edge_t	*removeedges[MAXHEIGHT];
extern	edge_FPM_t	*newedgesFPM[MAXHEIGHT];
extern	edge_FPM_t	*removeedgesFPM[MAXHEIGHT];

extern	int	screenwidth;

// FIXME: make stack vars when debugging done
extern	edge_t	edge_head;
extern	edge_t	edge_tail;
extern	edge_t	edge_aftertail;
extern int		r_bmodelactive;
extern vrect_t	*pconupdate;

extern float		aliasxscale, aliasyscale, aliasxcenter, aliasycenter;
extern fixedpoint_t		aliasxscaleFPM, aliasyscaleFPM, aliasxcenterFPM, aliasycenterFPM;
extern float		r_aliastransition, r_resfudge;
extern fixedpoint_t		r_aliastransitionFPM, r_resfudgeFPM;

extern int		r_outofsurfaces;
extern int		r_outofedges;

extern mvertex_t	*r_pcurrentvertbase;
extern mvertex_FPM_t	*r_pcurrentvertbaseFPM;
#ifdef USE_PQ_OPT2
extern mvertex_fxp_t	*r_pcurrentvertbase_fxp;
#endif
extern int			r_maxvalidedgeoffset;

void R_AliasClipTriangle (mtriangle_t *ptri);
void R_AliasClipTriangleFPM (mtriangle_t *ptri);

extern float	r_time1;
extern float	dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
extern float	se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;
extern int		r_frustum_indexes[4*6];
extern int		r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
extern qboolean	r_surfsonstack;
extern cshift_t	cshift_water;
extern qboolean	r_dowarpold, r_viewchanged;

extern mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern mleaf_FPM_t	*r_viewleafFPM, *r_oldviewleafFPM;

extern vec3_t		r_emins, r_emaxs;
extern vec3_FPM_t	r_eminsFPM, r_emaxsFPM;
extern mnode_t		*r_pefragtopnode;
extern mnode_FPM_t	*r_pefragtopnodeFPM;
extern int			r_clipflags;
extern int			r_dlightframecount;
extern qboolean		r_fov_greater_than_90;

void R_StoreEfrags (efrag_t **ppefrag);
void R_StoreEfragsFPM (efrag_FPM_t **ppefrag);
void R_TimeRefresh_f (void);
void R_TimeGraph (void);
void R_TimeGraphFPM (void);
void R_PrintAliasStats (void);
void R_PrintTimes (void);
void R_PrintDSpeeds (void);
void R_AnimateLight (void);
int R_LightPoint (vec3_t p);
int R_LightPointFPM (vec3_FPM_t p);
void R_SetupFrame (void);
void R_SetupFrameFPM (void);
void R_cshift_f (void);
void R_EmitEdge (mvertex_t *pv0, mvertex_t *pv1);
void R_EmitEdgeFPM (mvertex_FPM_t *pv0, mvertex_FPM_t *pv1);
//void R_ClipEdge (mvertex_t *pv0, mvertex_t *pv1, clipplane_t *clip); // made static inline
void R_SplitEntityOnNode2 (mnode_t *node);
void R_SplitEntityOnNode2FPM (mnode_FPM_t *node);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void R_MarkLightsFPM (dlight_FPM_t *light, int bit, mnode_FPM_t *node);

#endif
