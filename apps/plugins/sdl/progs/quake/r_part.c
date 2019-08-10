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

#include "quakedef.h"
#include "r_local.h"

#define MAX_PARTICLES			2048	// default max # of particles at one
										//  time
//Dan East: Reduced from 512 to 0
#define ABSOLUTE_MIN_PARTICLES	0		// no fewer than this no matter what's
										//  on the command line

int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

particle_t	*active_particles, *free_particles;

particle_t	*particles;
int			r_numparticles;
int			r_allocatedparticles;

vec3_t			r_pright, r_pup, r_ppn;

#define NUMVERTEXNORMALS	162
extern	float			r_avertexnormals[NUMVERTEXNORMALS][3];

#ifdef USEFPM
particle_FPM_t	*active_particlesFPM, *free_particlesFPM;
particle_FPM_t	*particlesFPM;
vec3_FPM_t		r_prightFPM, r_pupFPM, r_ppnFPM;
extern	fixedpoint_t	r_avertexnormalsFPM[NUMVERTEXNORMALS][3];
#endif //USEFPM


/*
===============
R_InitParticles
===============
*/
void R_InitParticles (void)
{
	int		i;

	i = COM_CheckParm ("-particles");

	if (i)
	{
		r_numparticles = (int)(Q_atoi(com_argv[i+1]));
		if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
			r_numparticles = ABSOLUTE_MIN_PARTICLES;
	}
	else
	{
		r_numparticles = (int)r_maxparticles.value;//MAX_PARTICLES;
	}

	r_allocatedparticles=r_numparticles;

	particles = (particle_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_t), "particles");
}

#ifdef USEFPM
void R_InitParticlesFPM (void)
{
	int		i;

	i = COM_CheckParm ("-particles");

	if (i)
	{
		r_numparticles = (int)(Q_atoi(com_argv[i+1]));
		if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
			r_numparticles = ABSOLUTE_MIN_PARTICLES;
	}
	else
	{
		r_numparticles = MAX_PARTICLES;
	}

	particlesFPM = (particle_FPM_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_FPM_t), "particles");

	//Dan: TODO: prebuild this table and load it in r_alias, instead of converting it
	//from the existing float table.
	for (i=0; i<NUMVERTEXNORMALS; i++) {
		r_avertexnormalsFPM[i][0]=FPM_FROMFLOAT(r_avertexnormals[i][0]);
		r_avertexnormalsFPM[i][1]=FPM_FROMFLOAT(r_avertexnormals[i][1]);
		r_avertexnormalsFPM[i][2]=FPM_FROMFLOAT(r_avertexnormals[i][2]);
	}
}
#endif //USEFPM

//Dan: Quake2 is not defined for our builds
#ifdef QUAKE2
void R_DarkFieldParticles (entity_t *ent)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	org[0] = ent->origin[0];
	org[1] = ent->origin[1];
	org[2] = ent->origin[2];
	for (i=-16 ; i<16 ; i+=8)
		for (j=-16 ; j<16 ; j+=8)
			for (k=0 ; k<32 ; k+=8)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 150 + rand()%6;
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalizeNoRet (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}
#endif


/*
===============
R_EntityParticles
===============
*/

vec3_t					avelocities[NUMVERTEXNORMALS];
#ifdef USEFPM
vec3_FPM_t				avelocitiesFPM[NUMVERTEXNORMALS];
fixedpoint_t			beamlengthFPM = FPM_FROMLONGC(16);
#endif //USEFPM
float					beamlength = 16;
vec3_t					avelocity = {23, 7, 3};
float					partstep = (float)0.01;
float					timescale = (float)0.01;

void R_EntityParticles (entity_t *ent)
{
	int			count;
	int			i;
	particle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist;
	
	dist = 64;
	count = 50;

if (!avelocities[0][0])
{
for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
avelocities[0][i] = (float)((rand()&255) * 0.01);
}


	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = (float)(cl.time * avelocities[i][0]);
		sy = (float)sin(angle);
		cy = (float)cos(angle);
		angle = (float)(cl.time * avelocities[i][1]);
		sp = (float)sin(angle);
		cp = (float)cos(angle);
		angle = (float)(cl.time * avelocities[i][2]);
		sr = (float)sin(angle);
		cr = (float)cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = (float)(cl.time + 0.01);
		p->color = 0x6f;
		p->type = pt_explode;
		
		p->org[0] = ent->origin[0] + r_avertexnormals[i][0]*dist + forward[0]*beamlength;			
		p->org[1] = ent->origin[1] + r_avertexnormals[i][1]*dist + forward[1]*beamlength;			
		p->org[2] = ent->origin[2] + r_avertexnormals[i][2]*dist + forward[2]*beamlength;			
	}
}

#ifdef USEFPM
void R_EntityParticlesFPM (entity_FPM_t *ent)
{
	int				count;
	int				i;
	particle_FPM_t	*p;
	fixedpoint_t	angle;
	fixedpoint_t	sr, sp, sy, cr, cp, cy;
	vec3_FPM_t		forward;
	fixedpoint_t	dist;
	
	dist = FPM_FROMLONG(64);
	count = 50;

	if (!avelocitiesFPM[0][0])
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocitiesFPM[0][i] = FPM_MUL(FPM_FROMLONG(rand()&255), FPM_FROMFLOAT(0.01));
	

	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		//Dan: TODO: Lots of float <-> fixedpoint conversions here.  We're lucky if we
		//break even instruction-wise with the original all-float routine.
		angle = FPM_MUL(FPM_FROMFLOAT(clFPM.time), avelocitiesFPM[i][0]);
		sy = FPM_SIN(angle);
		cy = FPM_COS(angle);
		angle = FPM_MUL(FPM_FROMFLOAT(clFPM.time), avelocitiesFPM[i][1]);
		sp = FPM_SIN(angle);
		cp = FPM_COS(angle);
		angle = FPM_MUL(FPM_FROMFLOAT(clFPM.time), avelocitiesFPM[i][2]);
		sr = FPM_SIN(angle);
		cr = FPM_COS(angle);
	
		forward[0] = FPM_MUL(cp,cy);
		forward[1] = FPM_MUL(cp,sy);
		forward[2] = -sp;

		if (!free_particlesFPM)
			return;
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;

		p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_FROMFLOAT(0.01));
		p->color = 0x6f;
		p->type = pt_explode;
		
		p->org[0] = FPM_ADD3(ent->origin[0], FPM_MUL(r_avertexnormalsFPM[i][0],dist), FPM_MUL(forward[0],beamlengthFPM));
		p->org[1] = FPM_ADD3(ent->origin[1], FPM_MUL(r_avertexnormalsFPM[i][1],dist), FPM_MUL(forward[1],beamlengthFPM));
		p->org[2] = FPM_ADD3(ent->origin[2], FPM_MUL(r_avertexnormalsFPM[i][2],dist), FPM_MUL(forward[2],beamlengthFPM));
	}
}
#endif //USEFPM

/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<r_allocatedparticles ; i++)
		particles[i].next = &particles[i+1];
	if (r_numparticles) 
		particles[r_numparticles-1].next = NULL;
	else free_particles = NULL;
}

#ifdef USEFPM
void R_ClearParticlesFPM (void)
{
	int		i;
	
	free_particlesFPM = &particlesFPM[0];
	active_particlesFPM = NULL;

	for (i=0 ;i<r_numparticles ; i++)
		particlesFPM[i].next = &particlesFPM[i+1];
	particlesFPM[r_numparticles-1].next = NULL;
}
#endif //USEFPM

void R_ReadPointFile_f (void)
{
	FILE	*f;
	vec3_t	org;
	int		r;
	int		c;
	particle_t	*p;
	char	name[MAX_OSPATH];
	
	sprintf (name,"maps/%s.pts", sv.name);

	COM_FOpenFile (name, &f);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
            r = fscanf (f,"%f ", &org[0]);
            r += fscanf (f,"%f ", &org[1]);
            r += fscanf (f,"%f\n", &org[2]);
		if (r != 3)
			break;
		c++;
		
		if (!free_particles)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		p->die = 99999;
		p->color = (float)((-c)&15);
		p->type = pt_static;
		VectorCopy (vec3_origin, p->vel);
		VectorCopy (org, p->org);
	}

	fclose (f);
	Con_Printf ("%i points read\n", c);
}

#ifdef USEFPM
void R_ReadPointFile_fFPM (void)
{
	FILE			*f;
	vec3_FPM_t		org;
	int				r;
	int				c;
	particle_FPM_t	*p;
	char			name[MAX_OSPATH];
	
	sprintf (name,"maps/%s.pts", sv.name);

	COM_FOpenFile (name, &f);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
		r = fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		if (!free_particlesFPM)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;
		
		p->die = FPM_FROMLONG(99999);
		p->color = ((-c)&15);
		p->type = pt_static;
		VectorCopy (vec3_originFPM, p->vel);

		//Dan: Here we convert the vector from the floats stored in the wad
		p->org[0]=FPM_FROMFLOAT(org[0]);
		p->org[1]=FPM_FROMFLOAT(org[1]);
		p->org[2]=FPM_FROMFLOAT(org[2]);
		//VectorCopy (org, p->org);
	}

	fclose (f);
	Con_Printf ("%i points read\n", c);
}
#endif //USEFPM

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = (float)(MSG_ReadChar () * (1.0/16));
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

if (msgcount == 255)
	count = 1024;
else
	count = msgcount;
	
	R_RunParticleEffect (org, dir, color, count);
}

#ifdef USEFPM
void R_ParseParticleEffectFPM (void)
{
	vec3_FPM_t	org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = FPM_FROMFLOAT(MSG_ReadCoord ());
	for (i=0 ; i<3 ; i++)
		dir[i] = FPM_MUL(FPM_FROMLONG(MSG_ReadChar ()), FPM_FROMFLOAT(1.0/16));
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

if (msgcount == 255)
	count = 1024;
else
	count = msgcount;
	
	R_RunParticleEffectFPM (org, dir, color, count);
}
#endif //USEFPM

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion (vec3_t org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = (float)(cl.time + 5);
		p->color = (float)ramp1[0];
		p->ramp = (float)(rand()&3);
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (float)((rand()%512)-256);
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (float)((rand()%512)-256);
			}
		}
	}
}

#ifdef USEFPM
void R_ParticleExplosionFPM (vec3_FPM_t org)
{
	int				i, j;
	particle_FPM_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particlesFPM)
			return;
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;

		p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_FROMLONG(5));
		p->color = ramp1[0];
		p->ramp = FPM_FROMLONG(rand()&3);
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
				p->vel[j] = FPM_FROMLONG((rand()%512)-256);
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
				p->vel[j] = FPM_FROMLONG((rand()%512)-256);
			}
		}
	}
}
#endif //USEFPM

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	particle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = (float)(cl.time + 0.3);
		p->color = (float)(colorStart + (colorMod % colorLength));
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (float)((rand()%512)-256);
		}
	}
}

#ifdef USEFPM
void R_ParticleExplosion2FPM (vec3_FPM_t org, int colorStart, int colorLength)
{
	int				i, j;
	particle_FPM_t	*p;
	int				colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!free_particlesFPM)
			return;
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;

		p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_FROMFLOAT(0.3));
		p->color = (colorStart + (colorMod % colorLength));
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
			p->vel[j] = FPM_FROMLONG((rand()%512)-256);
		}
	}
}
#endif //USEFPM

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion (vec3_t org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = (float)(cl.time + 1 + (rand()&8)*0.05);

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = (float)(66 + rand()%6);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (float)((rand()%512)-256);
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = (float)(150 + rand()%6);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (float)((rand()%512)-256);
			}
		}
	}
}

#ifdef USEFPM
void R_BlobExplosionFPM (vec3_FPM_t org)
{
	int				i, j;
	particle_FPM_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particlesFPM)
			return;
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;

		p->die = FPM_ADD3(FPM_FROMFLOAT(clFPM.time), 1, FPM_MUL(FPM_FROMLONG(rand()&8),FPM_FROMFLOAT(0.05)));

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = (66 + rand()%6);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
				p->vel[j] = FPM_FROMLONG((rand()%512)-256);
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = (150 + rand()%6);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
				p->vel[j] = FPM_FROMLONG((rand()%512)-256);
			}
		}
	}
}
#endif //USEFPM

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		if (count == 1024)
		{	// rocket explosion
			p->die = (float)(cl.time + 5);
			p->color = (float)ramp1[0];
			p->ramp = (float)(rand()&3);
			if (i & 1)
			{
				p->type = pt_explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()%32)-16);
					p->vel[j] = (float)((rand()%512)-256);
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()%32)-16);
					p->vel[j] = (float)((rand()%512)-256);
				}
			}
		}
		else
		{
			p->die = (float)(cl.time + 0.1*(rand()%5));
			p->color = (float)((color&~7) + (rand()&7));
			p->type = pt_slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&15)-8);
				p->vel[j] = dir[j]*15;// + (rand()%300)-150;
			}
		}
	}
}

#ifdef USEFPM
void R_RunParticleEffectFPM (vec3_FPM_t org, vec3_FPM_t dir, int color, int count)
{
	int				i, j;
	particle_FPM_t	*p;
	
	for (i=0 ; i<count ; i++)
	{
		if (!free_particlesFPM)
			return;
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;

		if (count == 1024)
		{	// rocket explosion
			p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_FROMLONG(5));
			p->color = ramp1[0];
			p->ramp = FPM_FROMLONG(rand()&3);
			if (i & 1)
			{
				p->type = pt_explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
					p->vel[j] = FPM_FROMLONG((rand()%512)-256);
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()%32)-16));
					p->vel[j] = FPM_FROMLONG((rand()%512)-256);
				}
			}
		}
		else
		{
			p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_MUL(FPM_FROMFLOAT(0.1),FPM_FROMLONG(rand()%5)));
			p->color = ((color&~7) + (rand()&7));
			p->type = pt_slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = FPM_ADD(org[j], FPM_FROMLONG((rand()&15)-8));
				p->vel[j] = FPM_MUL(dir[j],FPM_FROMLONG(15));// + (rand()%300)-150;
			}
		}
	}
}
#endif //USEFPM

/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = (float)(cl.time + 2 + (rand()&31) * 0.02);
				p->color = (float)(224 + (rand()&7));
				p->type = pt_slowgrav;
				
				dir[0] = (float)(j*8 + (rand()&7));
				dir[1] = (float)(i*8 + (rand()&7));
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = (float)(50 + (rand()&63));
				VectorScale (dir, vel, p->vel);
			}
}

#ifdef USEFPM
void R_LavaSplashFPM (vec3_FPM_t org)
{
	int				i, j, k;
	particle_FPM_t	*p;
	fixedpoint_t	vel;
	vec3_FPM_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!free_particlesFPM)
					return;
				p = free_particlesFPM;
				free_particlesFPM = p->next;
				p->next = active_particlesFPM;
				active_particlesFPM = p;
		
				p->die = FPM_ADD3(FPM_FROMFLOAT(clFPM.time), FPM_FROMLONG(2), FPM_MUL(FPM_FROMLONG(rand()&31), FPM_FROMFLOAT(0.02)));
				p->color = (224 + (rand()&7));
				p->type = pt_slowgrav;
				
				dir[0] = FPM_FROMLONG(j*8 + (rand()&7));
				dir[1] = FPM_FROMLONG(i*8 + (rand()&7));
				dir[2] = FPM_FROMLONG(256);
	
				p->org[0] = FPM_ADD(org[0], dir[0]);
				p->org[1] = FPM_ADD(org[1], dir[1]);
				p->org[2] = FPM_ADD(org[2], FPM_FROMLONG(rand()&63));
	
				VectorNormalizeFPM (dir);						
				vel = FPM_FROMLONG(50 + (rand()&63));
				VectorScaleFPM (dir, vel, p->vel);
			}
}
#endif //USEFPM

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = (float)(cl.time + 0.2 + (rand()&7) * 0.02);
				p->color = (float)(7 + (rand()&7));
				p->type = pt_slowgrav;
				
				dir[0] = (float)j*8;
				dir[1] = (float)i*8;
				dir[2] = (float)k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = (float)(50 + (rand()&63));
				VectorScale (dir, vel, p->vel);
			}
}

#ifdef USEFPM
void R_TeleportSplashFPM (vec3_FPM_t org)
{
	int				i, j, k;
	particle_FPM_t	*p;
	fixedpoint_t	vel;
	vec3_FPM_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!free_particlesFPM)
					return;
				p = free_particlesFPM;
				free_particlesFPM = p->next;
				p->next = active_particlesFPM;
				active_particlesFPM = p;
		
				p->die = FPM_ADD3(FPM_FROMFLOAT(clFPM.time), FPM_FROMFLOAT(0.2), FPM_MUL(FPM_FROMLONG(rand()&7), FPM_FROMFLOAT(0.02)));
				p->color = (7 + (rand()&7));
				p->type = pt_slowgrav;
				
				dir[0] = FPM_FROMLONG(j*8);
				dir[1] = FPM_FROMLONG(i*8);
				dir[2] = FPM_FROMLONG(k*8);
	
				p->org[0] = FPM_ADD(org[0], FPM_FROMLONG(i + (rand()&3)));
				p->org[1] = FPM_ADD(org[1], FPM_FROMLONG(j + (rand()&3)));
				p->org[2] = FPM_ADD(org[2], FPM_FROMLONG(k + (rand()&3)));
	
				VectorNormalizeFPM (dir);						
				vel = FPM_FROMLONG(50 + (rand()&63));
				VectorScaleFPM(dir, vel, p->vel);
			}
}
#endif //USEFPM

void R_RocketTrail (vec3_t start, vec3_t end, int type)
{
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	int			dec;
	static int	tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	if (type < 128)
		dec = 3;
	else
	{
		dec = 1;
		type -= 128;
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		VectorCopy (vec3_origin, p->vel);
		p->die = (float)(cl.time + 2);

		switch (type)
		{
			case 0:	// rocket trail
				p->ramp = (float)((rand()&3));
				p->color = (float)(ramp3[(int)p->ramp]);
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case 1:	// smoke smoke
				p->ramp = (float)((rand()&3) + 2);
				p->color = (float)(ramp3[(int)p->ramp]);
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case 2:	// blood
				p->type = pt_grav;
				p->color = (float)(67 + (rand()&3));
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case 3:
			case 5:	// tracer
				p->die = (float)(cl.time + 0.5);
				p->type = pt_static;
				if (type == 3)
					p->color = (float)(52 + ((tracercount&4)<<1));
				else
					p->color = (float)(230 + ((tracercount&4)<<1));
			
				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;

			case 4:	// slight blood
				p->type = pt_grav;
				p->color = (float)(67 + (rand()&3));
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				len -= 3;
				break;

			case 6:	// voor trail
				p->color = (float)(9*16 + 8 + (rand()&3));
				p->type = pt_static;
				p->die = (float)(cl.time + 0.3);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&15)-8);
				break;
		}
		

		VectorAdd (start, vec, start);
	}
}

#ifdef USEFPM
void R_RocketTrailFPM (vec3_FPM_t start, vec3_FPM_t end, int type)
{
	vec3_FPM_t		vec;
	fixedpoint_t	len;
	int				j;
	particle_FPM_t	*p;
	//Dan: changed dec from int to fixedpoint_t, so it is the same type as len
	fixedpoint_t	dec;
	static int		tracercount;

	VectorSubtractFPM (end, start, vec);
	len = VectorNormalizeFPM (vec);
	if (type < FPM_FROMLONG(128))
		dec = FPM_FROMLONG(3);
	else
	{
		dec = FPM_FROMLONG(1);
		type -= 128;
	}

	while (len > 0)
	{
		len=FPM_SUB(len, dec);

		if (!free_particlesFPM)
			return;
		p = free_particlesFPM;
		free_particlesFPM = p->next;
		p->next = active_particlesFPM;
		active_particlesFPM = p;
		
		VectorCopy(vec3_originFPM, p->vel);
		p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), 2);

		switch (type)
		{
			case 0:	// rocket trail
				p->ramp = FPM_FROMLONG((rand()&3));
				p->color = (ramp3[FPM_TOLONG(p->ramp)]);
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = FPM_ADD(start[j], FPM_FROMLONG((rand()%6)-3));
				break;

			case 1:	// smoke smoke
				p->ramp = FPM_FROMLONG((rand()&3) + 2);
				p->color = (ramp3[FPM_TOLONG(p->ramp)]);
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = FPM_ADD(start[j], FPM_FROMLONG((rand()%6)-3));
				break;

			case 2:	// blood
				p->type = pt_grav;
				p->color = (67 + (rand()&3));
				for (j=0 ; j<3 ; j++)
					p->org[j] = FPM_ADD(start[j], FPM_FROMLONG((rand()%6)-3));
				break;

			case 3:
			case 5:	// tracer
				p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_FROMFLOAT(0.5));
				p->type = pt_static;
				if (type == 3)
					p->color = (52 + ((tracercount&4)<<1));
				else
					p->color = (230 + ((tracercount&4)<<1));
			
				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = FPM_MUL(30,vec[1]);
					p->vel[1] = FPM_MUL(30,-vec[0]);
				}
				else
				{
					p->vel[0] = FPM_MUL(30,-vec[1]);
					p->vel[1] = FPM_MUL(30,vec[0]);
				}
				break;

			case 4:	// slight blood
				p->type = pt_grav;
				p->color = (67 + (rand()&3));
				for (j=0 ; j<3 ; j++)
					p->org[j] = FPM_ADD(start[j], FPM_FROMLONG((rand()%6)-3));
				len -= 3;
				break;

			case 6:	// voor trail
				p->color = (9*16 + 8 + (rand()&3));
				p->type = pt_static;
				p->die = FPM_ADD(FPM_FROMFLOAT(clFPM.time), FPM_FROMFLOAT(0.3));
				for (j=0 ; j<3 ; j++)
					p->org[j] = FPM_ADD(start[j], FPM_FROMLONG((rand()&15)-8));
				break;
		}
		

		VectorAddFPM (start, vec, start);
	}
}
#endif //USEFPM

/*
===============
R_DrawParticles
===============
*/
extern	cvar_t	sv_gravity;

void R_DrawParticles (void)
{
	particle_t		*p, *kill;
	float			grav;
	int				i;
	float			time2, time3;
	float			time1;
	float			dvel;
	float			frametime;
	
#ifdef GLQUAKE
	vec3_t			up, right;
	float			scale;

    GL_Bind(particletexture);
	glEnable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBegin (GL_TRIANGLES);

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);
#else
	D_StartParticles ();

	VectorScale (vright, xscaleshrink, r_pright);
	VectorScale (vup, yscaleshrink, r_pup);
	VectorCopy (vpn, r_ppn);
#endif
	frametime = (float)(cl.time - cl.oldtime);
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = (float)(frametime * sv_gravity.value * 0.05);
	dvel = 4*frametime;
	
	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

#ifdef GLQUAKE
		// hack a scale up to keep particles from disapearing
		scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];
		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;
		glColor3ubv ((byte *)&d_8to24table[(int)p->color]);
		glTexCoord2f (0,0);
		glVertex3fv (p->org);
		glTexCoord2f (1,0);
		glVertex3f (p->org[0] + up[0]*scale, p->org[1] + up[1]*scale, p->org[2] + up[2]*scale);
		glTexCoord2f (0,1);
		glVertex3f (p->org[0] + right[0]*scale, p->org[1] + right[1]*scale, p->org[2] + right[2]*scale);
#else
		D_DrawParticle (p);
#endif
		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = (float)(ramp3[(int)p->ramp]);
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = (float)(ramp1[(int)p->ramp]);
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = (float)(ramp2[(int)p->ramp]);
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_grav:
#ifdef QUAKE2
			p->vel[2] -= grav * 20;
			break;
#endif
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;
		}
	}

#ifdef GLQUAKE
	glEnd ();
	glDisable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#else
	D_EndParticles ();
#endif
}

#ifdef USEFPM
void R_DrawParticlesFPM (void)
{
	particle_FPM_t	*p, *kill;
	fixedpoint_t	grav;
	int				i;
	fixedpoint_t	time1, time2, time3;
	fixedpoint_t	dvel;
	fixedpoint_t	frametime;
	//Dan: var used for one-time fixedpoint conversion
	fixedpoint_t	cltime=FPM_FROMFLOAT(clFPM.time);
	
#ifdef GLQUAKE
	//Dan: not converted, unused by Pocket PC
	vec3_t			up, right;
	float			scale;

    GL_Bind(particletexture);
	glEnable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBegin (GL_TRIANGLES);

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);
#else
	//Dan: Empty function
	//D_StartParticlesFPM ();

	VectorScaleFPM (vrightFPM, xscaleshrinkFPM, r_prightFPM);
	VectorScaleFPM (vupFPM, yscaleshrinkFPM, r_pupFPM);
	VectorCopy (vpn, r_ppn);
#endif
	frametime = FPM_SUB(cltime, FPM_FROMFLOAT(clFPM.oldtime));
	time3 = FPM_MUL(frametime, FPM_FROMLONG(15));
	time2 = FPM_MUL(frametime, FPM_FROMLONG(10)); // 15;
	time1 = FPM_MUL(frametime, FPM_FROMLONG(5));
	grav =  FPM_MUL(FPM_MUL(frametime, FPM_FROMFLOAT(sv_gravity.value)),  FPM_FROMFLOAT(0.05));
	dvel = FPM_MUL(FPM_FROMLONGC(4), frametime);
	
	for ( ;; ) 
	{
		kill = active_particlesFPM;
		if (kill && kill->die < cltime)
		{
			active_particlesFPM = kill->next;
			kill->next = free_particlesFPM;
			free_particlesFPM = kill;
			continue;
		}
		break;
	}

	for (p=active_particlesFPM ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cltime)
			{
				p->next = kill->next;
				kill->next = free_particlesFPM;
				free_particlesFPM = kill;
				continue;
			}
			break;
		}

#ifdef GLQUAKE
		// hack a scale up to keep particles from disapearing
		scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];
		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;
		glColor3ubv ((byte *)&d_8to24table[(int)p->color]);
		glTexCoord2f (0,0);
		glVertex3fv (p->org);
		glTexCoord2f (1,0);
		glVertex3f (p->org[0] + up[0]*scale, p->org[1] + up[1]*scale, p->org[2] + up[2]*scale);
		glTexCoord2f (0,1);
		glVertex3f (p->org[0] + right[0]*scale, p->org[1] + right[1]*scale, p->org[2] + right[2]*scale);
#else
		D_DrawParticleFPM (p);
#endif
		p->org[0] = FPM_ADD(p->org[0], FPM_MUL(p->vel[0],frametime));
		p->org[1] = FPM_ADD(p->org[1], FPM_MUL(p->vel[1],frametime));
		p->org[2] = FPM_ADD(p->org[2], FPM_MUL(p->vel[2],frametime));
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp = FPM_ADD(p->ramp, time1);
			if (p->ramp >= FPM_FROMLONGC(6))
				p->die = FPM_FROMLONGC(-1);
			else
				p->color = (ramp3[FPM_TOLONG(p->ramp)]);
			p->vel[2] = FPM_ADD(p->vel[2], grav);
			break;

		case pt_explode:
			p->ramp = FPM_ADD(p->ramp, time2);
			if (p->ramp >=FPM_FROMLONGC(8))
				p->die = FPM_FROMLONGC(-1);
			else
				p->color = (ramp1[FPM_TOLONG(p->ramp)]);
			for (i=0 ; i<3 ; i++)
				p->vel[i] = FPM_ADD(p->vel[i], FPM_MUL(p->vel[i], dvel));
			p->vel[2] = FPM_SUB(p->vel[2], grav);
			break;

		case pt_explode2:
			p->ramp = FPM_ADD(p->ramp, time3);
			if (p->ramp >= FPM_FROMLONGC(8))
				p->die = FPM_FROMLONGC(-1);
			else
				p->color = (ramp2[FPM_TOLONG(p->ramp)]);
			for (i=0 ; i<3 ; i++)
				p->vel[i] = FPM_SUB(p->vel[i], FPM_MUL(p->vel[i],frametime));
			p->vel[2] = FPM_SUB(p->vel[2], grav);
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] = FPM_ADD(p->vel[i], FPM_MUL(p->vel[i],dvel));
			p->vel[2] = FPM_SUB(p->vel[2], grav);
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] = FPM_SUB(p->vel[i], FPM_MUL(p->vel[i],dvel));
			p->vel[2] = FPM_SUB(p->vel[2], grav);
			break;

		case pt_grav:
#ifdef QUAKE2
			p->vel[2] = FPM_SUB(p->vel[2], FPM_MUL(grav, 20));
			break;
#endif
		case pt_slowgrav:
			p->vel[2] = FPM_SUB(p->vel[2], grav);
			break;
		}
	}

#ifdef GLQUAKE
	glEnd ();
	glDisable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#else
	//Dan: Empty function
	//D_EndParticlesFPM ();
#endif
}
#endif //USEFPM
