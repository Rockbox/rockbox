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

// r_draw.c

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"	// FIXME: shouldn't need to include this

#define MAXLEFTCLIPEDGES		100

// !!! if these are changed, they must be changed in asm_draw.h too !!!
#define FULLY_CLIPPED_CACHED	0x80000000
#define FRAMECOUNT_MASK			0x7FFFFFFF

unsigned int	cacheoffset;

int			c_faceclip;					// number of faces clipped

zpointdesc_t	r_zpointdesc;

polydesc_t		r_polydesc;



clipplane_t	*entity_clipplanes;
clipplane_t	view_clipplanes[4];
clipplane_t	world_clipplanes[16];

#ifdef USE_PQ_OPT2
clipplane_fxp_t		view_clipplanes_fxp[4];
#endif

medge_t			*r_pedge;

qboolean		r_leftclipped, r_rightclipped;
static qboolean	makeleftedge, makerightedge;
qboolean		r_nearzionly;

int		sintable[SIN_BUFFER_SIZE];
int		intsintable[SIN_BUFFER_SIZE];

mvertex_t	r_leftenter, r_leftexit;
mvertex_t	r_rightenter, r_rightexit;

#ifdef USE_PQ_OPT2
mvertex_fxp_t	r_leftenter_fxp, r_leftexit_fxp;
mvertex_fxp_t	r_rightenter_fxp, r_rightexit_fxp;
#endif

typedef struct
{
	float	u,v;
	int		ceilv;
} evert_t;

int				r_emitted;
float			r_nearzi;
float			r_u1, r_v1, r_lzi1;
int				r_ceilv1;

#ifdef USE_PQ_OPT1
int				r_u1_fxp, r_v1_fxp, r_lzi1_fxp;
extern int		modelorg_fxp[3];
#endif

qboolean	r_lastvertvalid;


#if	!id386

/*
================
R_EmitEdge
================
*/
void R_EmitEdge (mvertex_t *pv0, mvertex_t *pv1)
{
	edge_t	*edge, *pcheck;
	int		u_check;
	float	u, u_step;
	vec3_t	local, transformed;
	float	*world;
	int		v, v2, ceilv0;
	float	scale, lzi0, u0, v0;
	int		side;

	if (r_lastvertvalid)
	{
		u0 = r_u1;
		v0 = r_v1;
		lzi0 = r_lzi1;
		ceilv0 = r_ceilv1;
	}
	else
	{
		world = &pv0->position[0];
	
	// transform and project
		VectorSubtract (world, modelorg, local);
		TransformVector (local, transformed);
	
		if (transformed[2] < NEAR_CLIP)
			transformed[2] = NEAR_CLIP;
	
		lzi0 = 1.0 / transformed[2];
	
	// FIXME: build x/yscale into transform?
		scale = xscale * lzi0;
		u0 = (xcenter + scale*transformed[0]);
		if (u0 < r_refdef.fvrectx_adj)
			u0 = r_refdef.fvrectx_adj;
		if (u0 > r_refdef.fvrectright_adj)
			u0 = r_refdef.fvrectright_adj;
	
		scale = yscale * lzi0;
		v0 = (ycenter - scale*transformed[1]);
		if (v0 < r_refdef.fvrecty_adj)
			v0 = r_refdef.fvrecty_adj;
		if (v0 > r_refdef.fvrectbottom_adj)
			v0 = r_refdef.fvrectbottom_adj;
	
		ceilv0 = (int) ceil(v0);
	}

	world = &pv1->position[0];

// transform and project
	VectorSubtract (world, modelorg, local);
	TransformVector (local, transformed);

	if (transformed[2] < NEAR_CLIP)
		transformed[2] = NEAR_CLIP;

	r_lzi1 = 1.0 / transformed[2];

	scale = xscale * r_lzi1;
	r_u1 = (xcenter + scale*transformed[0]);
	if (r_u1 < r_refdef.fvrectx_adj)
		r_u1 = r_refdef.fvrectx_adj;
	if (r_u1 > r_refdef.fvrectright_adj)
		r_u1 = r_refdef.fvrectright_adj;

	scale = yscale * r_lzi1;
	r_v1 = (ycenter - scale*transformed[1]);
	if (r_v1 < r_refdef.fvrecty_adj)
		r_v1 = r_refdef.fvrecty_adj;
	if (r_v1 > r_refdef.fvrectbottom_adj)
		r_v1 = r_refdef.fvrectbottom_adj;

	if (r_lzi1 > lzi0)
		lzi0 = r_lzi1;

	if (lzi0 > r_nearzi)	// for mipmap finding
		r_nearzi = lzi0;

// for right edges, all we want is the effect on 1/z
	if (r_nearzionly)
		return;

	r_emitted = 1;

	r_ceilv1 = (int) ceil(r_v1);


// create the edge
	if (ceilv0 == r_ceilv1)
	{
	// we cache unclipped horizontal edges as fully clipped
		if (cacheoffset != 0x7FFFFFFF)
		{
			cacheoffset = FULLY_CLIPPED_CACHED |
					(r_framecount & FRAMECOUNT_MASK);
		}

		return;		// horizontal edge
	}

	side = ceilv0 > r_ceilv1;

	edge = edge_p++;

	edge->owner = r_pedge;

	edge->nearzi = lzi0;

	if (side == 0)
	{
	// trailing edge (go from p1 to p2)
		v = ceilv0;
		v2 = r_ceilv1 - 1;

		edge->surfs[0] = surface_p - surfaces;
		edge->surfs[1] = 0;

		u_step = ((r_u1 - u0) / (r_v1 - v0));
		u = u0 + ((float)v - v0) * u_step;
	}
	else
	{
	// leading edge (go from p2 to p1)
		v2 = ceilv0 - 1;
		v = r_ceilv1;

		edge->surfs[0] = 0;
		edge->surfs[1] = surface_p - surfaces;

		u_step = ((u0 - r_u1) / (v0 - r_v1));
		u = r_u1 + ((float)v - r_v1) * u_step;
	}

	edge->u_step = u_step*0x100000;
	edge->u = u*0x100000 + 0xFFFFF;

// we need to do this to avoid stepping off the edges if a very nearly
// horizontal edge is less than epsilon above a scan, and numeric error causes
// it to incorrectly extend to the scan, and the extension of the line goes off
// the edge of the screen
// FIXME: is this actually needed?
	if (edge->u < r_refdef.vrect_x_adj_shift20)
		edge->u = r_refdef.vrect_x_adj_shift20;
	if (edge->u > r_refdef.vrectright_adj_shift20)
		edge->u = r_refdef.vrectright_adj_shift20;

//
// sort the edge in normally
//
	u_check = edge->u;
	if (edge->surfs[0])
		u_check++;	// sort trailers after leaders

	if (!newedges[v] || newedges[v]->u >= u_check)
	{
		edge->next = newedges[v];
		newedges[v] = edge;
	}
	else
	{
		pcheck = newedges[v];
		while (pcheck->next && pcheck->next->u < u_check)
			pcheck = pcheck->next;
		edge->next = pcheck->next;
		pcheck->next = edge;
	}

	edge->nextremove = removeedges[v2];
	removeedges[v2] = edge;
}

#ifdef USE_PQ_OPT2
void R_EmitEdgeFXP_fxp (mvertex_fxp_t *pv0, mvertex_fxp_t *pv1)
{
	//This is just like R_EmitEdge_fxp, except that the vertex parameters are
	//already fixed point, and don't need to be converted.

	edge_t	*edge, *pcheck;
	int		u_check;
	//float	u, u_step;
	int		u_fxp, u_step_fxp;
	//vec3_t	local, transformed;
	int		local_fxp[3], transformed_fxp[3];
	int		*world;
	int		v, v2, ceilv0;
	//float	scale, lzi0, u0, v0;
	int		scale_fxp, scale2_fxp, lzi0_fxp, u0_fxp, v0_fxp;
	int		side;

	if (r_lastvertvalid)
	{
		u0_fxp = r_u1_fxp;
		v0_fxp = r_v1_fxp;
		lzi0_fxp = r_lzi1_fxp;
		//lzi0 = r_lzi1;
		ceilv0 = r_ceilv1;
	}
	else
	{
		//world_fxp=(int)(pv0->position[0]*(float)(2^16));
		world = &pv0->position[0];
		
	// transform and project
		//VectorSubtract (world, modelorg, local);
		//Vector Subtract (and convert)
		local_fxp[0]=world[0]-modelorg_fxp[0];
		local_fxp[1]=world[1]-modelorg_fxp[1];
		local_fxp[2]=world[2]-modelorg_fxp[2];

		//TransformVector (local, transformed);
		//transformed_fxp[0] = (int)(local_fxp[0]*vright[0])+(int)(local_fxp[1]*vright[1])+(int)(local_fxp[2]*vright[2]);
		//transformed_fxp[1] = (int)(local_fxp[0]*vup[0])+(int)(local_fxp[1]*vup[1])+(int)(local_fxp[2]*vup[2]);
		//transformed_fxp[2] = (int)(local_fxp[0]*vpn[0])+(int)(local_fxp[1]*vpn[1])+(int)(local_fxp[2]*vpn[2]);

		//13.19 / 24.8 = 21.11
		transformed_fxp[0] = local_fxp[0]/vright_fxp[0]+local_fxp[1]/vright_fxp[1]+local_fxp[2]/vright_fxp[2];
		transformed_fxp[1] = local_fxp[0]/vup_fxp[0]+local_fxp[1]/vup_fxp[1]+local_fxp[2]/vup_fxp[2];
		transformed_fxp[2] = local_fxp[0]/vpn_fxp[0]+local_fxp[1]/vpn_fxp[1]+local_fxp[2]/vpn_fxp[2];
		
		if (transformed_fxp[2] < (int)(NEAR_CLIP*2048.0))
			transformed_fxp[2] = (int)(NEAR_CLIP*2048.0);

		transformed_fxp[0]/=16;		//21.11->25.7
		transformed_fxp[1]/=16;		//21.11->25.7
		transformed_fxp[2]/=8;		//21.11->24.8

		lzi0_fxp=transformed_fxp[2];
		//lzi0 = (float)(1.0 / transformed[2]);

	// FIXME: build x/yscale into transform?
		//scale = xscale * lzi0;
		//u0 = (xcenter + scale*transformed[0]);

		//Dan: Is this needed?
		if (!transformed_fxp[2]) scale_fxp=0;
		else scale_fxp=xscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
		scale2_fxp=transformed_fxp[0]*(scale_fxp);	// 25.7 * 17.15 = 10.22

		if (transformed_fxp[0]<0) {
			if (scale2_fxp>0) scale2_fxp=-511*4194304;
		} else {
			if (scale2_fxp<0) scale2_fxp=511*4194304;
		}

		u0_fxp=scale2_fxp+xcenter_fxp;

		if (u0_fxp < r_refdef_fvrectx_adj_fxp)
			u0_fxp = r_refdef_fvrectx_adj_fxp;
		if (u0_fxp > r_refdef_fvrectright_adj_fxp)
			u0_fxp = r_refdef_fvrectright_adj_fxp;

		//scale = yscale * lzi0;
		//v0 = (ycenter - scale*transformed[1]);

		//Dan: Is this needed?
		if (!transformed_fxp[2]) scale_fxp=0;
		else scale_fxp=yscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
		scale2_fxp=transformed_fxp[1]*(scale_fxp);	// 25.7 * 17.15 = 10.22

		if (transformed_fxp[1]<0) {
			if (scale2_fxp>0) scale2_fxp=-511*4194304;
		} else {
			if (scale2_fxp<0) scale2_fxp=511*4194304;	//255*8388608;
		}

		v0_fxp = ycenter_fxp-scale2_fxp;

		if (v0_fxp < r_refdef_fvrecty_adj_fxp)
			v0_fxp = r_refdef_fvrecty_adj_fxp;
		if (v0_fxp > r_refdef_fvrectbottom_adj_fxp)
			v0_fxp = r_refdef_fvrectbottom_adj_fxp;
	
		ceilv0 = v0_fxp/4194304;
		if (v0_fxp&0x3FFFFF) ceilv0++;
	}

	//world(pv1->position[0]*(float)(2^16));
	world = &pv1->position[0];

// transform and project
	//VectorSubtract (world, modelorg, local);
	//Vector Subtract (and convert)
	local_fxp[0]=world[0]-modelorg_fxp[0];
	local_fxp[1]=world[1]-modelorg_fxp[1];
	local_fxp[2]=world[2]-modelorg_fxp[2];

	//TransformVector (local, transformed);
	//transformed_fxp[0] = ((int)(local_fxp[0]*vright[0]))+((int)(local_fxp[1]*vright[1]))+((int)(local_fxp[2]*vright[2]));
	//transformed_fxp[1] = ((int)(local_fxp[0]*vup[0]))+((int)(local_fxp[1]*vup[1]))+((int)(local_fxp[2]*vup[2]));
	//transformed_fxp[2] = ((int)(local_fxp[0]*vpn[0]))+((int)(local_fxp[1]*vpn[1]))+((int)(local_fxp[2]*vpn[2]));

	transformed_fxp[0] = local_fxp[0]/vright_fxp[0]+local_fxp[1]/vright_fxp[1]+local_fxp[2]/vright_fxp[2];
	transformed_fxp[1] = local_fxp[0]/vup_fxp[0]+local_fxp[1]/vup_fxp[1]+local_fxp[2]/vup_fxp[2];
	transformed_fxp[2] = local_fxp[0]/vpn_fxp[0]+local_fxp[1]/vpn_fxp[1]+local_fxp[2]/vpn_fxp[2];

	if (transformed_fxp[2] < (int)(NEAR_CLIP*2048.0))
		transformed_fxp[2] = (int)(NEAR_CLIP*2048.0);

	transformed_fxp[0]/=16;
	transformed_fxp[1]/=16;
	transformed_fxp[2]/=8;

	r_lzi1_fxp=transformed_fxp[2];
	//r_lzi1 = (float)(1.0 / transformed[2]);
	//scale = xscale * r_lzi1;

	//Dan: Is this needed?
	if (!transformed_fxp[2]) scale_fxp=0;
	else scale_fxp=xscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
	scale2_fxp=transformed_fxp[0]*(scale_fxp);	// 24.8 * 17.15 = 9.23 //21.11

	if (transformed_fxp[0]<0) {
		if (scale2_fxp>0) scale2_fxp=-511*4194304;
	} else {
		if (scale2_fxp<0) scale2_fxp=511*4194304;
	}

	//r_u1 = (xcenter + scale*transformed[0]);
	r_u1_fxp = xcenter_fxp + scale2_fxp;
	if (r_u1_fxp < r_refdef_fvrectx_adj_fxp)
		r_u1_fxp = r_refdef_fvrectx_adj_fxp;
	if (r_u1_fxp > r_refdef_fvrectright_adj_fxp)
		r_u1_fxp = r_refdef_fvrectright_adj_fxp;

	//scale = yscale * r_lzi1;
	//r_v1 = (ycenter - scale*transformed[1]);

	//Dan: Is this needed?
	if (!transformed_fxp[2]) scale_fxp=0;
	else scale_fxp=yscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
	scale2_fxp=transformed_fxp[1]*(scale_fxp);	// 23.9 * 17.15 = 9.23 //21.11

	if (transformed_fxp[1]<0) {
		if (scale2_fxp>0) scale2_fxp=-511*4194304;
	} else {
		if (scale2_fxp<0) scale2_fxp=511*4194304;
	}

	r_v1_fxp = ycenter_fxp - scale2_fxp;
	if (r_v1_fxp < r_refdef_fvrecty_adj_fxp)
		r_v1_fxp = r_refdef_fvrecty_adj_fxp;
	if (r_v1_fxp > r_refdef_fvrectbottom_adj_fxp)
		r_v1_fxp = r_refdef_fvrectbottom_adj_fxp;

	//if (r_lzi1 > lzi0)
	//	lzi0 = r_lzi1;
	if (r_lzi1_fxp < lzi0_fxp)
		lzi0_fxp = r_lzi1_fxp;

	//if (lzi0 > r_nearzi)	// for mipmap finding
	//	r_nearzi = lzi0;
	if (/*128*/128.0/lzi0_fxp > r_nearzi) {	// for mipmap finding
		//if (!lzi0_fxp) r_nearzi=0;
		//else
			r_nearzi = (float)(128.0/lzi0_fxp);
	}

// for right edges, all we want is the effect on 1/z
	if (r_nearzionly)
		return;

	r_emitted = 1;

	//r_ceilv1 = (int) ceil(r_v1);
	r_ceilv1 = r_v1_fxp/4194304;
	if (r_v1_fxp&0x3FFFFF) r_ceilv1++;


// create the edge
	if (ceilv0 == r_ceilv1)
	{
	// we cache unclipped horizontal edges as fully clipped
		if (cacheoffset != 0x7FFFFFFF)
		{
			cacheoffset = FULLY_CLIPPED_CACHED |
					(r_framecount & FRAMECOUNT_MASK);
		}

		return;		// horizontal edge
	}

	side = ceilv0 > r_ceilv1;
	
	edge = edge_p++;

	edge->owner = NULL;
	
	edge->owner = r_pedge;

	//Dan: Is this needed?
	if (!lzi0_fxp) edge->nearzi=0.0;
	else edge->nearzi = (float)(128.0f/*256.0*//lzi0_fxp);

	if (side == 0)
	{
		int tmp;
	// trailing edge (go from p1 to p2)
		v = ceilv0;
		v2 = r_ceilv1 - 1;

		edge->surfs[0] = surface_p - surfaces;
		edge->surfs[1] = 0;

		//u_step = ((r_u1 - u0) / (r_v1 - v0));
		//u = u0 + ((float)v - v0) * u_step;

		tmp=((r_v1_fxp - v0_fxp)>>10);
		if (tmp)
			u_step_fxp=(r_u1_fxp - u0_fxp) / tmp;	//10.22 / 15.12 = 22.10
		else
			u_step_fxp=0;
		u_fxp = u0_fxp + ((v*4194304 - v0_fxp)>>12 * u_step_fxp>>12);
	}
	else
	{
		int tmp;
	// leading edge (go from p2 to p1)
		v2 = ceilv0 - 1;
		v = r_ceilv1;

		edge->surfs[0] = 0;
		edge->surfs[1] = surface_p - surfaces;

		//u_step = ((u0 - r_u1) / (v0 - r_v1));
		//u = r_u1 + ((float)v - r_v1) * u_step;

		tmp=((v0_fxp - r_v1_fxp)>>10);
		if (tmp)
			u_step_fxp = (u0_fxp - r_u1_fxp) / tmp;
		else 
			u_step_fxp=0;
		u_fxp = r_u1_fxp + ((v*4194304 - r_v1_fxp)>>12 * u_step_fxp>>12);

	}
	//edge->u_step = tmp*0x100000;
	//edge->u = (int)(/*(u_fxp/65536)*0x100000*/u_fxp/4 + 0xFFFFF);

	edge->u_step = u_step_fxp*1024;///16; //tmp*0x100000;
	edge->u = (int)(/*(u_fxp/65536)*0x100000*/u_fxp/4 + 0xFFFFF);

// we need to do this to avoid stepping off the edges if a very nearly
// horizontal edge is less than epsilon above a scan, and numeric error causes
// it to incorrectly extend to the scan, and the extension of the line goes off
// the edge of the screen
// FIXME: is this actually needed?
	if (edge->u < r_refdef.vrect_x_adj_shift20)
		edge->u = r_refdef.vrect_x_adj_shift20;
	if (edge->u > r_refdef.vrectright_adj_shift20)
		edge->u = r_refdef.vrectright_adj_shift20;

//
// sort the edge in normally
//
	u_check = edge->u;
	if (edge->surfs[0])
		u_check++;	// sort trailers after leaders

	if (!newedges[v] || newedges[v]->u >= u_check)
	{
		edge->next = newedges[v];
		newedges[v] = edge;
	}
	else
	{
		pcheck = newedges[v];
		while (pcheck->next && pcheck->next->u < u_check)
			pcheck = pcheck->next;
		edge->next = pcheck->next;
		pcheck->next = edge;
	}

	edge->nextremove = removeedges[v2];
	removeedges[v2] = edge;
}
#endif

#ifdef USE_PQ_OPT1
void R_EmitEdge_fxp (mvertex_t *pv0, mvertex_t *pv1)
{
	edge_t	*edge, *pcheck;
	int		u_check;
	//float	u, u_step;
	int		u_fxp, u_step_fxp;
	//vec3_t	local, transformed;
	int		local_fxp[3], transformed_fxp[3];
	float	*world;
	int		v, v2, ceilv0;
	//float	scale, lzi0, u0, v0;
	int		scale_fxp, scale2_fxp, lzi0_fxp, u0_fxp, v0_fxp;
	int		side;

	if (r_lastvertvalid)
	{
		u0_fxp = r_u1_fxp;
		v0_fxp = r_v1_fxp;
		lzi0_fxp = r_lzi1_fxp;
		//lzi0 = r_lzi1;
		ceilv0 = r_ceilv1;
	}
	else
	{
		//world_fxp=(int)(pv0->position[0]*(float)(2^16));
		world = &pv0->position[0];
		
	// transform and project
		//VectorSubtract (world, modelorg, local);
		//Vector Subtract (and convert)
		local_fxp[0]=((int)(world[0]*(524288.0)))-modelorg_fxp[0];
		local_fxp[1]=((int)(world[1]*(524288.0)))-modelorg_fxp[1];
		local_fxp[2]=((int)(world[2]*(524288.0)))-modelorg_fxp[2];

		//TransformVector (local, transformed);
		//transformed_fxp[0] = (int)(local_fxp[0]*vright[0])+(int)(local_fxp[1]*vright[1])+(int)(local_fxp[2]*vright[2]);
		//transformed_fxp[1] = (int)(local_fxp[0]*vup[0])+(int)(local_fxp[1]*vup[1])+(int)(local_fxp[2]*vup[2]);
		//transformed_fxp[2] = (int)(local_fxp[0]*vpn[0])+(int)(local_fxp[1]*vpn[1])+(int)(local_fxp[2]*vpn[2]);

		transformed_fxp[0] = local_fxp[0]/vright_fxp[0]+local_fxp[1]/vright_fxp[1]+local_fxp[2]/vright_fxp[2];
		//transformed_fxp[0]*=256;
		transformed_fxp[1] = local_fxp[0]/vup_fxp[0]+local_fxp[1]/vup_fxp[1]+local_fxp[2]/vup_fxp[2];
		//transformed_fxp[1]*=256;
		transformed_fxp[2] = local_fxp[0]/vpn_fxp[0]+local_fxp[1]/vpn_fxp[1]+local_fxp[2]/vpn_fxp[2];
		transformed_fxp[2]*=256;
		
		if (transformed_fxp[2] < (int)(NEAR_CLIP*1048576.0))
			transformed_fxp[2] = (int)(NEAR_CLIP*1048576.0);

		transformed_fxp[0]/=16;
		transformed_fxp[1]/=16;
		transformed_fxp[2]/=2048;

		lzi0_fxp=transformed_fxp[2];
		//lzi0 = (float)(1.0 / transformed[2]);

	// FIXME: build x/yscale into transform?
		//scale = xscale * lzi0;
		//u0 = (xcenter + scale*transformed[0]);

		scale_fxp=xscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
		scale2_fxp=transformed_fxp[0]*(scale_fxp);	// 25.7 * 17.15 = 10.22

		if (transformed_fxp[0]<0) {
			if (scale2_fxp>0) scale2_fxp=-511*4194304;
		} else {
			if (scale2_fxp<0) scale2_fxp=511*4194304;
		}

		u0_fxp=scale2_fxp+xcenter_fxp;

		if (u0_fxp < r_refdef_fvrectx_adj_fxp)
			u0_fxp = r_refdef_fvrectx_adj_fxp;
		if (u0_fxp > r_refdef_fvrectright_adj_fxp)
			u0_fxp = r_refdef_fvrectright_adj_fxp;

		//scale = yscale * lzi0;
		//v0 = (ycenter - scale*transformed[1]);
		scale_fxp=yscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
		scale2_fxp=transformed_fxp[1]*(scale_fxp);	// 25.7 * 17.15 = 10.22

		if (transformed_fxp[1]<0) {
			if (scale2_fxp>0) scale2_fxp=-511*4194304;
		} else {
			if (scale2_fxp<0) scale2_fxp=511*4194304;	//255*8388608;
		}

		v0_fxp = ycenter_fxp-scale2_fxp;

		if (v0_fxp < r_refdef_fvrecty_adj_fxp)
			v0_fxp = r_refdef_fvrecty_adj_fxp;
		if (v0_fxp > r_refdef_fvrectbottom_adj_fxp)
			v0_fxp = r_refdef_fvrectbottom_adj_fxp;
	
		ceilv0 = v0_fxp/4194304;
		if (v0_fxp&0x3FFFFF) ceilv0++;
	}

	//world(pv1->position[0]*(float)(2^16));
	world = &pv1->position[0];

// transform and project
	//VectorSubtract (world, modelorg, local);
	//Vector Subtract (and convert)
	local_fxp[0]=((int)(world[0]*(524288.0)))-modelorg_fxp[0];
	local_fxp[1]=((int)(world[1]*(524288.0)))-modelorg_fxp[1];
	local_fxp[2]=((int)(world[2]*(524288.0)))-modelorg_fxp[2];

	//TransformVector (local, transformed);
	//transformed_fxp[0] = ((int)(local_fxp[0]*vright[0]))+((int)(local_fxp[1]*vright[1]))+((int)(local_fxp[2]*vright[2]));
	//transformed_fxp[1] = ((int)(local_fxp[0]*vup[0]))+((int)(local_fxp[1]*vup[1]))+((int)(local_fxp[2]*vup[2]));
	//transformed_fxp[2] = ((int)(local_fxp[0]*vpn[0]))+((int)(local_fxp[1]*vpn[1]))+((int)(local_fxp[2]*vpn[2]));

	transformed_fxp[0] = local_fxp[0]/vright_fxp[0]+local_fxp[1]/vright_fxp[1]+local_fxp[2]/vright_fxp[2];
	//transformed_fxp[0]*=256;
	transformed_fxp[1] = local_fxp[0]/vup_fxp[0]+local_fxp[1]/vup_fxp[1]+local_fxp[2]/vup_fxp[2];
	//transformed_fxp[1]*=256;
	transformed_fxp[2] = local_fxp[0]/vpn_fxp[0]+local_fxp[1]/vpn_fxp[1]+local_fxp[2]/vpn_fxp[2];
	transformed_fxp[2]*=256;

	//transformed_fxp[2]=-transformed_fxp[2];
	//if (transformed[2] < NEAR_CLIP)
	//	transformed[2] = (float)NEAR_CLIP;
	if (transformed_fxp[2] < (int)(NEAR_CLIP*524288.0))
		transformed_fxp[2] = (int)(NEAR_CLIP*524288.0);

	transformed_fxp[0]/=16;
	transformed_fxp[1]/=16;
	transformed_fxp[2]/=2048;

	r_lzi1_fxp=transformed_fxp[2];
	//r_lzi1 = (float)(1.0 / transformed[2]);
	//scale = xscale * r_lzi1;

	scale_fxp=xscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
	scale2_fxp=transformed_fxp[0]*(scale_fxp);	// 24.8 * 17.15 = 9.23 //21.11

	if (transformed_fxp[0]<0) {
		if (scale2_fxp>0) scale2_fxp=-511*4194304;
	} else {
		if (scale2_fxp<0) scale2_fxp=511*4194304;
	}

	//r_u1 = (xcenter + scale*transformed[0]);
	r_u1_fxp = xcenter_fxp + scale2_fxp;
	if (r_u1_fxp < r_refdef_fvrectx_adj_fxp)
		r_u1_fxp = r_refdef_fvrectx_adj_fxp;
	if (r_u1_fxp > r_refdef_fvrectright_adj_fxp)
		r_u1_fxp = r_refdef_fvrectright_adj_fxp;

	//scale = yscale * r_lzi1;
	//r_v1 = (ycenter - scale*transformed[1]);

	scale_fxp=yscale_fxp/transformed_fxp[2];	//9.23 / 24.8 = 17.15
	scale2_fxp=transformed_fxp[1]*(scale_fxp);	// 23.9 * 17.15 = 9.23 //21.11

	if (transformed_fxp[1]<0) {
		if (scale2_fxp>0) scale2_fxp=-511*4194304;
	} else {
		if (scale2_fxp<0) scale2_fxp=511*4194304;
	}

	r_v1_fxp = ycenter_fxp - scale2_fxp;
	if (r_v1_fxp < r_refdef_fvrecty_adj_fxp)
		r_v1_fxp = r_refdef_fvrecty_adj_fxp;
	if (r_v1_fxp > r_refdef_fvrectbottom_adj_fxp)
		r_v1_fxp = r_refdef_fvrectbottom_adj_fxp;

	//if (r_lzi1 > lzi0)
	//	lzi0 = r_lzi1;
	if (r_lzi1_fxp < lzi0_fxp)
		lzi0_fxp = r_lzi1_fxp;

	//if (lzi0 > r_nearzi)	// for mipmap finding
	//	r_nearzi = lzi0;
	if (128.0/lzi0_fxp > r_nearzi) {	// for mipmap finding
		//if (!lzi0_fxp) r_nearzi=0;
		//else
			r_nearzi = (float)(128.0/lzi0_fxp);
	}

// for right edges, all we want is the effect on 1/z
	if (r_nearzionly)
		return;

	r_emitted = 1;

	//r_ceilv1 = (int) ceil(r_v1);
	r_ceilv1 = r_v1_fxp/4194304;
	if (r_v1_fxp&0x3FFFFF) r_ceilv1++;


// create the edge
	if (ceilv0 == r_ceilv1)
	{
	// we cache unclipped horizontal edges as fully clipped
		if (cacheoffset != 0x7FFFFFFF)
		{
			cacheoffset = FULLY_CLIPPED_CACHED |
					(r_framecount & FRAMECOUNT_MASK);
		}

		return;		// horizontal edge
	}

	side = ceilv0 > r_ceilv1;
	
	edge = edge_p++;

	edge->owner = NULL;
	
	edge->owner = r_pedge;

	edge->nearzi = (float)(128.0/lzi0_fxp);

	{
	//float tmp;
	if (side == 0)
	{
		//int tmp;
	// trailing edge (go from p1 to p2)
		v = ceilv0;
		v2 = r_ceilv1 - 1;

		edge->surfs[0] = surface_p - surfaces;
		edge->surfs[1] = 0;

		//u_step = ((r_u1 - u0) / (r_v1 - v0));
		//u = u0 + ((float)v - v0) * u_step;

		u_step_fxp=(r_u1_fxp - u0_fxp) / ((r_v1_fxp - v0_fxp)>>10);	//10.22 / 15.12 = 22.10
		u_fxp = u0_fxp + ((v*4194304 - v0_fxp)>>12 * u_step_fxp>>12);

		//tmp=(((r_u1_fxp - u0_fxp)/8388608.0) / ((r_v1_fxp - v0_fxp)/8388608.0));
		//u_step_fxp=(int)(tmp*8388608.0);
		//u_fxp = u0_fxp + (((float)v - v0_fxp/8388608.0) * tmp)*8388608.0;
	}
	else
	{
		//int tmp;
	// leading edge (go from p2 to p1)
		v2 = ceilv0 - 1;
		v = r_ceilv1;

		edge->surfs[0] = 0;
		edge->surfs[1] = surface_p - surfaces;

		//u_step = ((u0 - r_u1) / (v0 - r_v1));
		//u = r_u1 + ((float)v - r_v1) * u_step;

		u_step_fxp = (u0_fxp - r_u1_fxp) / ((v0_fxp - r_v1_fxp)>>10);
		u_fxp = r_u1_fxp + ((v*4194304 - r_v1_fxp)>>12 * u_step_fxp>>12);

		//tmp=(((u0_fxp - r_u1_fxp)/8388608.0) / ((v0_fxp - r_v1_fxp)/8388608.0));
		//u_step_fxp = (int)(tmp*8388608.0);
		//u_fxp = r_u1_fxp + (((float)v - r_v1_fxp/8388608.0) * tmp)*8388608.0;
	}
	//edge->u_step = tmp*0x100000;
	//edge->u = (int)(/*(u_fxp/65536)*0x100000*/u_fxp/4 + 0xFFFFF);

	edge->u_step = u_step_fxp*1024;///16; //tmp*0x100000;
	edge->u = (int)(/*(u_fxp/65536)*0x100000*/u_fxp/4 + 0xFFFFF);
}

// we need to do this to avoid stepping off the edges if a very nearly
// horizontal edge is less than epsilon above a scan, and numeric error causes
// it to incorrectly extend to the scan, and the extension of the line goes off
// the edge of the screen
// FIXME: is this actually needed?
	if (edge->u < r_refdef.vrect_x_adj_shift20)
		edge->u = r_refdef.vrect_x_adj_shift20;
	if (edge->u > r_refdef.vrectright_adj_shift20)
		edge->u = r_refdef.vrectright_adj_shift20;

//
// sort the edge in normally
//
	u_check = edge->u;
	if (edge->surfs[0])
		u_check++;	// sort trailers after leaders

	if (!newedges[v] || newedges[v]->u >= u_check)
	{
		edge->next = newedges[v];
		newedges[v] = edge;
	}
	else
	{
		pcheck = newedges[v];
		while (pcheck->next && pcheck->next->u < u_check)
			pcheck = pcheck->next;
		edge->next = pcheck->next;
		pcheck->next = edge;
	}

	edge->nextremove = removeedges[v2];
	removeedges[v2] = edge;
}
#endif

#ifdef USE_PQ_OPT2
void R_ClipEdge_fxp (mvertex_fxp_t pv0[3], mvertex_fxp_t pv1[3], clipplane_fxp_t *clip)
{
	int				d0_fxp, d1_fxp, f_fxp;
	mvertex_fxp_t	clipvert_fxp;
	//12.20

	if (clip)
	{
		do
		{
			//13.19 / 20.12 = 25.7
			d0_fxp=(pv0->position[0])/clip->normal[0]+(pv0->position[1])/clip->normal[1]+(pv0->position[2])/clip->normal[2];
			d1_fxp=(pv1->position[0])/clip->normal[0]+(pv1->position[1])/clip->normal[1]+(pv1->position[2])/clip->normal[2];
			d0_fxp-=(clip->dist);
			d1_fxp-=(clip->dist);
			//d0 = DotProduct (pv0->position, clip->normal) - clip->dist;
			//d1 = DotProduct (pv1->position, clip->normal) - clip->dist;

			if (d0_fxp >= 0)
			{
			// point 0 is unclipped
				if (d1_fxp >= 0)
				{
				// both points are unclipped
					continue;
				}

			// only point 1 is clipped

			// we don't cache clipped edges
				cacheoffset = 0x7FFFFFFF;

				if (!(d0_fxp)) 
					f_fxp=2<<29;	
				else {
					f_fxp = (((d0_fxp - d1_fxp)<<7)/(d0_fxp));	//(25.7->18.14) / 25.7 = 25.7
					if (!f_fxp) f_fxp=2<<29;
				}


				clipvert_fxp.position[0] = pv0->position[0] +
						(((pv1->position[0] - pv0->position[0])/f_fxp)<<7); //13.19 / 25.7 = 20.12

				clipvert_fxp.position[1] = pv0->position[1] +
						(((pv1->position[1] - pv0->position[1])/f_fxp)<<7);

				clipvert_fxp.position[2] = pv0->position[2] +
						(((pv1->position[2] - pv0->position[2])/f_fxp)<<7);

				if (clip->leftedge)
				{
					r_leftclipped = true;
					r_leftexit_fxp = clipvert_fxp;
				}
				else if (clip->rightedge)
				{
					r_rightclipped = true;
					r_rightexit_fxp = clipvert_fxp;
				}

				R_ClipEdge_fxp (pv0, &clipvert_fxp, clip->next);
				return;
			}
			else
			{
			// point 0 is clipped
				if (d1_fxp < 0)
				{
				// both points are clipped
				// we do cache fully clipped edges
					if (!r_leftclipped)
						cacheoffset = FULLY_CLIPPED_CACHED |
								(r_framecount & FRAMECOUNT_MASK);
					return;
				}

			// only point 0 is clipped
				r_lastvertvalid = false;

			// we don't cache partially clipped edges
				cacheoffset = 0x7FFFFFFF;

				if (!d0_fxp) 
					f_fxp=2<<29;	
				else {
					f_fxp = (((d0_fxp - d1_fxp)<<7)/d0_fxp);	//12.20 / 20.12 = 24.8
					if (!f_fxp) f_fxp=2<<29;
				}

				clipvert_fxp.position[0] = pv0->position[0] +
						(((pv1->position[0] - pv0->position[0])/f_fxp)<<7); //12.20 / 24.8 = 20.12

				clipvert_fxp.position[1] = pv0->position[1] +
						(((pv1->position[1] - pv0->position[1])/f_fxp)<<7);

				clipvert_fxp.position[2] = pv0->position[2] +
						(((pv1->position[2] - pv0->position[2])/f_fxp)<<7);

				if (clip->leftedge)
				{
					r_leftclipped = true;
					r_leftenter_fxp = clipvert_fxp;
				}
				else if (clip->rightedge)
				{
					r_rightclipped = true;
					r_rightenter_fxp = clipvert_fxp;
				}

				R_ClipEdge_fxp (&clipvert_fxp, pv1, clip->next);
				return;
			}
		} while ((clip = clip->next) != NULL);
	}
/*
{
	mvertex_t p0, p1;
	p0.position[0]=pv0->position[0]/524288.0f;
	p0.position[1]=pv0->position[1]/524288.0f;
	p0.position[2]=pv0->position[2]/524288.0f;
	p1.position[0]=pv1->position[0]/524288.0f;
	p1.position[1]=pv1->position[1]/524288.0f;
	p1.position[2]=pv1->position[2]/524288.0f;
	R_EmitEdge (&p0, &p1);
}
*/
// add the edge
	R_EmitEdgeFXP_fxp (pv0, pv1);
}
#endif

/*
================
R_ClipEdge
================
*/
static inline void R_ClipEdge (mvertex_t *pv0, mvertex_t *pv1, clipplane_t *clip)
{
	float		d0, d1, f;
	mvertex_t	clipvert;

	if (clip)
	{
		do
		{
			d0 = DotProduct (pv0->position, clip->normal) - clip->dist;
			d1 = DotProduct (pv1->position, clip->normal) - clip->dist;

			if (d0 >= 0)
			{
			// point 0 is unclipped
				if (d1 >= 0)
				{
				// both points are unclipped
					continue;
				}

			// only point 1 is clipped

			// we don't cache clipped edges
				cacheoffset = 0x7FFFFFFF;

				f = d0 / (d0 - d1);
				clipvert.position[0] = pv0->position[0] +
						f * (pv1->position[0] - pv0->position[0]);
				clipvert.position[1] = pv0->position[1] +
						f * (pv1->position[1] - pv0->position[1]);
				clipvert.position[2] = pv0->position[2] +
						f * (pv1->position[2] - pv0->position[2]);

				if (clip->leftedge)
				{
					r_leftclipped = true;
					r_leftexit = clipvert;
				}
				else if (clip->rightedge)
				{
					r_rightclipped = true;
					r_rightexit = clipvert;
				}

				R_ClipEdge (pv0, &clipvert, clip->next);
				return;
			}
			else
			{
			// point 0 is clipped
				if (d1 < 0)
				{
				// both points are clipped
				// we do cache fully clipped edges
					if (!r_leftclipped)
						cacheoffset = FULLY_CLIPPED_CACHED |
								(r_framecount & FRAMECOUNT_MASK);
					return;
				}

			// only point 0 is clipped
				r_lastvertvalid = false;

			// we don't cache partially clipped edges
				cacheoffset = 0x7FFFFFFF;

				f = d0 / (d0 - d1);
				clipvert.position[0] = pv0->position[0] +
						f * (pv1->position[0] - pv0->position[0]);
				clipvert.position[1] = pv0->position[1] +
						f * (pv1->position[1] - pv0->position[1]);
				clipvert.position[2] = pv0->position[2] +
						f * (pv1->position[2] - pv0->position[2]);

				if (clip->leftedge)
				{
					r_leftclipped = true;
					r_leftenter = clipvert;
				}
				else if (clip->rightedge)
				{
					r_rightclipped = true;
					r_rightenter = clipvert;
				}

				R_ClipEdge (&clipvert, pv1, clip->next);
				return;
			}
		} while ((clip = clip->next) != NULL);
	}

// add the edge
#ifndef USE_PQ_OPT1
	R_EmitEdge (pv0, pv1);
#else
	R_EmitEdge_fxp (pv0, pv1);
#endif
}

#endif	// !id386


/*
================
R_EmitCachedEdge
================
*/
void R_EmitCachedEdge (void)
{
	edge_t		*pedge_t;

	pedge_t = (edge_t *)((unsigned long)r_edges + r_pedge->cachededgeoffset);

	if (!pedge_t->surfs[0])
		pedge_t->surfs[0] = surface_p - surfaces;
	else
		pedge_t->surfs[1] = surface_p - surfaces;

	if (pedge_t->nearzi > r_nearzi)	// for mipmap finding
		r_nearzi = pedge_t->nearzi;

	r_emitted = 1;
}


/*
================
R_RenderFace
================
*/
void R_RenderFace (msurface_t *fa, int clipflags)
{
	int			i, lindex;
	unsigned	mask;
	mplane_t	*pplane;
	float		distinv;
	vec3_t		p_normal;
	medge_t		*pedges, tedge;
	clipplane_t	*pclip;
#ifdef USE_PQ_OPT2
	clipplane_fxp_t	*pclip_fxp;
#endif

// skip out if no more surfs
	if ((surface_p) >= surf_max)
	{
		r_outofsurfaces++;
		return;
	}

// ditto if not enough edges left, or switch to auxedges if possible
	if ((edge_p + fa->numedges + 4) >= edge_max)
	{
		r_outofedges += fa->numedges;
		return;
	}

	c_faceclip++;

// set up clip planes
	pclip = NULL;

	for (i=3, mask = 0x08 ; i>=0 ; i--, mask >>= 1)
	{
		if (clipflags & mask)
		{
			view_clipplanes[i].next = pclip;
			pclip = &view_clipplanes[i];
		}
	}

#ifdef USE_PQ_OPT2
	pclip_fxp = NULL;

	for (i=3, mask = 0x08 ; i>=0 ; i--, mask >>= 1)
	{
		if (clipflags & mask)
		{
			view_clipplanes_fxp[i].next = pclip_fxp;
			pclip_fxp = &view_clipplanes_fxp[i];
		}
	}
#endif

// push the edges through
	r_emitted = 0;
	r_nearzi = 0;
	r_nearzionly = false;
	makeleftedge = makerightedge = false;
	pedges = currententity->model->edges;
	r_lastvertvalid = false;

	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = currententity->model->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];

		// if the edge is cached, we can just reuse the edge
			if (!insubmodel)
			{
				if (r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED)
				{
					if ((r_pedge->cachededgeoffset & FRAMECOUNT_MASK) ==
						r_framecount)
					{
						r_lastvertvalid = false;
						continue;
					}
				}
				else
				{
					if ((((unsigned long)edge_p - (unsigned long)r_edges) >
						 r_pedge->cachededgeoffset) &&
						(((edge_t *)((unsigned long)r_edges +
						 r_pedge->cachededgeoffset))->owner == r_pedge))
					{
						R_EmitCachedEdge ();
						r_lastvertvalid = false;
						continue;
					}
				}
			}

		// assume it's cacheable
			cacheoffset = (byte *)edge_p - (byte *)r_edges;
			r_leftclipped = r_rightclipped = false;
#ifndef USE_PQ_OPT2
			R_ClipEdge (&r_pcurrentvertbase[r_pedge->v[0]],
						&r_pcurrentvertbase[r_pedge->v[1]],
						pclip);
#else
			R_ClipEdge_fxp (&r_pcurrentvertbase_fxp[r_pedge->v[0]],
						&r_pcurrentvertbase_fxp[r_pedge->v[1]],
						pclip_fxp);
#endif

			r_pedge->cachededgeoffset = cacheoffset;

			if (r_leftclipped)
				makeleftedge = true;
			if (r_rightclipped)
				makerightedge = true;
			r_lastvertvalid = true;
		}
		else
		{
			lindex = -lindex;
			r_pedge = &pedges[lindex];
		// if the edge is cached, we can just reuse the edge
			if (!insubmodel)
			{
				if (r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED)
				{
					if ((r_pedge->cachededgeoffset & FRAMECOUNT_MASK) ==
						r_framecount)
					{
						r_lastvertvalid = false;
						continue;
					}
				}
				else
				{
				// it's cached if the cached edge is valid and is owned
				// by this medge_t
					if ((((unsigned long)edge_p - (unsigned long)r_edges) >
						 r_pedge->cachededgeoffset) &&
						(((edge_t *)((unsigned long)r_edges +
						 r_pedge->cachededgeoffset))->owner == r_pedge))
					{
						R_EmitCachedEdge ();
						r_lastvertvalid = false;
						continue;
					}
				}
			}

		// assume it's cacheable
			cacheoffset = (byte *)edge_p - (byte *)r_edges;
			r_leftclipped = r_rightclipped = false;
#ifndef USE_PQ_OPT2
			R_ClipEdge (&r_pcurrentvertbase[r_pedge->v[1]],
						&r_pcurrentvertbase[r_pedge->v[0]],
						pclip);
#else
			R_ClipEdge_fxp (&r_pcurrentvertbase_fxp[r_pedge->v[1]],
						&r_pcurrentvertbase_fxp[r_pedge->v[0]],
						pclip_fxp);
#endif
			r_pedge->cachededgeoffset = cacheoffset;

			if (r_leftclipped)
				makeleftedge = true;
			if (r_rightclipped)
				makerightedge = true;
			r_lastvertvalid = true;
		}
	}

// if there was a clip off the left edge, add that edge too
// FIXME: faster to do in screen space?
// FIXME: share clipped edges?
	if (makeleftedge)
	{
		r_pedge = &tedge;
		r_lastvertvalid = false;
#ifndef USE_PQ_OPT2
		R_ClipEdge (&r_leftexit, &r_leftenter, pclip->next);
#else
		R_ClipEdge_fxp (&r_leftexit_fxp, &r_leftenter_fxp, pclip_fxp->next);
#endif
	}

// if there was a clip off the right edge, get the right r_nearzi
	if (makerightedge)
	{
		r_pedge = &tedge;
		r_lastvertvalid = false;
		r_nearzionly = true;
#ifndef USE_PQ_OPT2
		R_ClipEdge (&r_rightexit, &r_rightenter, view_clipplanes[1].next);
#else
		R_ClipEdge_fxp (&r_rightexit_fxp, &r_rightenter_fxp, view_clipplanes_fxp[1].next);
#endif
	}

// if no edges made it out, return without posting the surface
	if (!r_emitted)
		return;

	r_polycount++;

	surface_p->data = (void *)fa;
	surface_p->nearzi = r_nearzi;
	surface_p->flags = fa->flags;
	surface_p->insubmodel = insubmodel;
	surface_p->spanstate = 0;
	surface_p->entity = currententity;
	surface_p->key = r_currentkey++;
	surface_p->spans = NULL;

	pplane = fa->plane;
// FIXME: cache this?
	TransformVector (pplane->normal, p_normal);
// FIXME: cache this?
	distinv = (float) 1.0 / (pplane->dist - DotProduct (modelorg, pplane->normal));

#ifndef USE_PQ_OPT3
	surface_p->d_zistepu = p_normal[0] * xscaleinv * distinv;
	surface_p->d_zistepv = -p_normal[1] * yscaleinv * distinv;
	surface_p->d_ziorigin = p_normal[2] * distinv -
			xcenter * surface_p->d_zistepu -
			ycenter * surface_p->d_zistepv;
#else
	surface_p->d_zistepu_fxp=(int)(p_normal[0] * xscaleinv * distinv*4194304.0f);
	surface_p->d_zistepv_fxp=(int)(-p_normal[1] * yscaleinv * distinv*4194304.0f);
	surface_p->d_ziorigin_fxp=((int)(p_normal[2] * distinv * 4194304.0f)) -
			((int)(xcenter * surface_p->d_zistepu_fxp)) -
			((int)(ycenter * surface_p->d_zistepv_fxp));
#endif
        
//JDC	VectorCopy (r_worldmodelorg, surface_p->modelorg);
	surface_p++;
}


/*
================
R_RenderBmodelFace
================
*/
void R_RenderBmodelFace (bedge_t *pedges, msurface_t *psurf)
{
	int			i;
	unsigned	mask;
	mplane_t	*pplane;
	float		distinv;
	vec3_t		p_normal;
	medge_t		tedge;
	clipplane_t	*pclip;

// skip out if no more surfs
	if (surface_p >= surf_max)
	{
		r_outofsurfaces++;
		return;
	}

// ditto if not enough edges left, or switch to auxedges if possible
	if ((edge_p + psurf->numedges + 4) >= edge_max)
	{
		r_outofedges += psurf->numedges;
		return;
	}

	c_faceclip++;

// this is a dummy to give the caching mechanism someplace to write to
	r_pedge = &tedge;

// set up clip planes
	pclip = NULL;

	for (i=3, mask = 0x08 ; i>=0 ; i--, mask >>= 1)
	{
		if (r_clipflags & mask)
		{
			view_clipplanes[i].next = pclip;
			pclip = &view_clipplanes[i];
		}
	}

// push the edges through
	r_emitted = 0;
	r_nearzi = 0;
	r_nearzionly = false;
	makeleftedge = makerightedge = false;
// FIXME: keep clipped bmodel edges in clockwise order so last vertex caching
// can be used?
	r_lastvertvalid = false;

	for ( ; pedges ; pedges = pedges->pnext)
	{
		r_leftclipped = r_rightclipped = false;
		R_ClipEdge (pedges->v[0], pedges->v[1], pclip);

		if (r_leftclipped)
			makeleftedge = true;
		if (r_rightclipped)
			makerightedge = true;
	}

// if there was a clip off the left edge, add that edge too
// FIXME: faster to do in screen space?
// FIXME: share clipped edges?
	if (makeleftedge)
	{
		r_pedge = &tedge;
		R_ClipEdge (&r_leftexit, &r_leftenter, pclip->next);
	}

// if there was a clip off the right edge, get the right r_nearzi
	if (makerightedge)
	{
		r_pedge = &tedge;
		r_nearzionly = true;
		R_ClipEdge (&r_rightexit, &r_rightenter, view_clipplanes[1].next);
	}

// if no edges made it out, return without posting the surface
	if (!r_emitted)
		return;

	r_polycount++;

	surface_p->data = (void *)psurf;
	surface_p->nearzi = r_nearzi;
	surface_p->flags = psurf->flags;
	surface_p->insubmodel = true;
	surface_p->spanstate = 0;
	surface_p->entity = currententity;
	surface_p->key = r_currentbkey;
	surface_p->spans = NULL;

	pplane = psurf->plane;
// FIXME: cache this?
	TransformVector (pplane->normal, p_normal);
// FIXME: cache this?
	distinv = 1.0 / (pplane->dist - DotProduct (modelorg, pplane->normal));

	surface_p->d_zistepu = p_normal[0] * xscaleinv * distinv;
	surface_p->d_zistepv = -p_normal[1] * yscaleinv * distinv;
	surface_p->d_ziorigin = p_normal[2] * distinv -
			xcenter * surface_p->d_zistepu -
			ycenter * surface_p->d_zistepv;

//JDC	VectorCopy (r_worldmodelorg, surface_p->modelorg);
	surface_p++;
}


/*
================
R_RenderPoly
================
*/
void R_RenderPoly (msurface_t *fa, int clipflags)
{
	int			i, lindex, lnumverts, s_axis, t_axis;
	float		dist, lastdist, lzi, scale, u, v, frac;
	unsigned	mask;
	vec3_t		local, transformed;
	clipplane_t	*pclip;
	medge_t		*pedges;
	mplane_t	*pplane;
	mvertex_t	verts[2][100];	//FIXME: do real number
	polyvert_t	pverts[100];	//FIXME: do real number, safely
	int			vertpage, newverts, newpage, lastvert;
	qboolean	visible;

// FIXME: clean this up and make it faster
// FIXME: guard against running out of vertices

	s_axis = t_axis = 0;	// keep compiler happy

// set up clip planes
	pclip = NULL;

	for (i=3, mask = 0x08 ; i>=0 ; i--, mask >>= 1)
	{
		if (clipflags & mask)
		{
			view_clipplanes[i].next = pclip;
			pclip = &view_clipplanes[i];
		}
	}

// reconstruct the polygon
// FIXME: these should be precalculated and loaded off disk
	pedges = currententity->model->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currententity->model->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			verts[0][i] = r_pcurrentvertbase[r_pedge->v[0]];
		}
		else
		{
			r_pedge = &pedges[-lindex];
			verts[0][i] = r_pcurrentvertbase[r_pedge->v[1]];
		}
	}

// clip the polygon, done if not visible
	while (pclip)
	{
		lastvert = lnumverts - 1;
		lastdist = DotProduct (verts[vertpage][lastvert].position,
							   pclip->normal) - pclip->dist;

		visible = false;
		newverts = 0;
		newpage = vertpage ^ 1;

		for (i=0 ; i<lnumverts ; i++)
		{
			dist = DotProduct (verts[vertpage][i].position, pclip->normal) -
					pclip->dist;

			if ((lastdist > 0) != (dist > 0))
			{
				frac = dist / (dist - lastdist);
				verts[newpage][newverts].position[0] =
						verts[vertpage][i].position[0] +
						((verts[vertpage][lastvert].position[0] -
						  verts[vertpage][i].position[0]) * frac);
				verts[newpage][newverts].position[1] =
						verts[vertpage][i].position[1] +
						((verts[vertpage][lastvert].position[1] -
						  verts[vertpage][i].position[1]) * frac);
				verts[newpage][newverts].position[2] =
						verts[vertpage][i].position[2] +
						((verts[vertpage][lastvert].position[2] -
						  verts[vertpage][i].position[2]) * frac);
				newverts++;
			}

			if (dist >= 0)
			{
				verts[newpage][newverts] = verts[vertpage][i];
				newverts++;
				visible = true;
			}

			lastvert = i;
			lastdist = dist;
		}

		if (!visible || (newverts < 3))
			return;

		lnumverts = newverts;
		vertpage ^= 1;
		pclip = pclip->next;
	}

// transform and project, remembering the z values at the vertices and
// r_nearzi, and extract the s and t coordinates at the vertices
	pplane = fa->plane;
	switch (pplane->type)
	{
	case PLANE_X:
	case PLANE_ANYX:
		s_axis = 1;
		t_axis = 2;
		break;
	case PLANE_Y:
	case PLANE_ANYY:
		s_axis = 0;
		t_axis = 2;
		break;
	case PLANE_Z:
	case PLANE_ANYZ:
		s_axis = 0;
		t_axis = 1;
		break;
	}

	r_nearzi = 0;

	for (i=0 ; i<lnumverts ; i++)
	{
	// transform and project
		VectorSubtract (verts[vertpage][i].position, modelorg, local);
		TransformVector (local, transformed);

		if (transformed[2] < NEAR_CLIP)
			transformed[2] = NEAR_CLIP;

		lzi = 1.0 / transformed[2];

		if (lzi > r_nearzi)	// for mipmap finding
			r_nearzi = lzi;

	// FIXME: build x/yscale into transform?
		scale = xscale * lzi;
		u = (xcenter + scale*transformed[0]);
		if (u < r_refdef.fvrectx_adj)
			u = r_refdef.fvrectx_adj;
		if (u > r_refdef.fvrectright_adj)
			u = r_refdef.fvrectright_adj;

		scale = yscale * lzi;
		v = (ycenter - scale*transformed[1]);
		if (v < r_refdef.fvrecty_adj)
			v = r_refdef.fvrecty_adj;
		if (v > r_refdef.fvrectbottom_adj)
			v = r_refdef.fvrectbottom_adj;

		pverts[i].u = u;
		pverts[i].v = v;
		pverts[i].zi = lzi;
		pverts[i].s = verts[vertpage][i].position[s_axis];
		pverts[i].t = verts[vertpage][i].position[t_axis];
	}

// build the polygon descriptor, including fa, r_nearzi, and u, v, s, t, and z
// for each vertex
	r_polydesc.numverts = lnumverts;
	r_polydesc.nearzi = r_nearzi;
	r_polydesc.pcurrentface = fa;
	r_polydesc.pverts = pverts;

// draw the polygon
	D_DrawPoly ();
}


/*
================
R_ZDrawSubmodelPolys
================
*/
void R_ZDrawSubmodelPolys (model_t *pmodel)
{
	int			i, numsurfaces;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;

	psurf = &pmodel->surfaces[pmodel->firstmodelsurface];
	numsurfaces = pmodel->nummodelsurfaces;

	for (i=0 ; i<numsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
		// FIXME: use bounding-box-based frustum clipping info?
			R_RenderPoly (psurf, 15);
		}
	}
}

