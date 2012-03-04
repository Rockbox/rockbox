/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001, 2002 Miodrag Vallat and others - see file
	AUTHORS for complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mloader.c,v 1.3 2005/04/07 19:57:39 realtech Exp $

  These routines are used to access the available module loaders

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>

#include "mikmod_internals.h"

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

MREADER *modreader;
MODULE of;

static	MLOADER *firstloader=NULL;

UWORD finetune[16]={
	8363,8413,8463,8529,8581,8651,8723,8757,
	7895,7941,7985,8046,8107,8169,8232,8280
};

MIKMODAPI CHAR* MikMod_InfoLoader(void)
{
	int len=0;
	MLOADER *l;
	CHAR *list=NULL;

	MUTEX_LOCK(lists);
	/* compute size of buffer */
	for(l=firstloader;l;l=l->next) len+=1+(l->next?1:0)+strlen(l->version);

	if(len)
		if((list=MikMod_malloc(len*sizeof(CHAR)))) {
			list[0]=0;
			/* list all registered module loders */
			for(l=firstloader;l;l=l->next)
				sprintf(list,(l->next)?"%s%s\n":"%s%s",list,l->version);
		}
	MUTEX_UNLOCK(lists);
	return list;
}

void _mm_registerloader(MLOADER* ldr)
{
	MLOADER *cruise=firstloader;

	if(cruise) {
		while(cruise->next) cruise = cruise->next;
		cruise->next=ldr;
	} else
		firstloader=ldr; 
}

MIKMODAPI void MikMod_RegisterLoader(struct MLOADER* ldr)
{
	/* if we try to register an invalid loader, or an already registered loader,
	   ignore this attempt */
	if ((!ldr)||(ldr->next))
		return;

	MUTEX_LOCK(lists);
	_mm_registerloader(ldr);
	MUTEX_UNLOCK(lists);
}

int ReadComment(UWORD len)
{
	if(len) {
		int i;

		if(!(of.comment=(CHAR*)MikMod_malloc(len+1))) return 0;
		_mm_read_UBYTES(of.comment,len,modreader);
		
		/* translate IT linefeeds */
		for(i=0;i<len;i++)
			if(of.comment[i]=='\r') of.comment[i]='\n';

		of.comment[len]=0;	/* just in case */
	}
	if(!of.comment[0]) {
		MikMod_free(of.comment);
		of.comment=NULL;
	}
	return 1;
}

int ReadLinedComment(UWORD len,UWORD linelen)
{
	CHAR *tempcomment,*line,*storage;
	UWORD total=0,t,lines;
	int i;

	lines = (len + linelen - 1) / linelen;
	if (len) {
		if(!(tempcomment=(CHAR*)MikMod_malloc(len+1))) return 0;
		if(!(storage=(CHAR*)MikMod_malloc(linelen+1))) {
			MikMod_free(tempcomment);
			return 0;
		}
		memset(tempcomment, ' ', len);
		_mm_read_UBYTES(tempcomment,len,modreader);

		/* compute message length */
		for(line=tempcomment,total=t=0;t<lines;t++,line+=linelen) {
			for(i=linelen;(i>=0)&&(line[i]==' ');i--) line[i]=0;
			for(i=0;i<linelen;i++) if (!line[i]) break;
			total+=1+i;
		}

		if(total>lines) {
			if(!(of.comment=(CHAR*)MikMod_malloc(total+1))) {
				MikMod_free(storage);
				MikMod_free(tempcomment);
				return 0;
			}

			/* convert message */
			for(line=tempcomment,t=0;t<lines;t++,line+=linelen) {
				for(i=0;i<linelen;i++) if(!(storage[i]=line[i])) break;
				storage[i]=0; /* if (i==linelen) */
				strcat(of.comment,storage);strcat(of.comment,"\r");
			}
			MikMod_free(storage);
			MikMod_free(tempcomment);
		}
	}
	return 1;
}

int AllocPositions(int total)
{
	if(!total) {
		_mm_errno=MMERR_NOT_A_MODULE;
		return 0;
	}
	if(!(of.positions=MikMod_calloc(total,sizeof(UWORD)))) return 0;
	return 1;
}

int AllocPatterns(void)
{
	int s,t,tracks = 0;

	if((!of.numpat)||(!of.numchn)) {
		_mm_errno=MMERR_NOT_A_MODULE;
		return 0;
	}
	/* Allocate track sequencing array */
	if(!(of.patterns=(UWORD*)MikMod_calloc((ULONG)(of.numpat+1)*of.numchn,sizeof(UWORD)))) return 0;
	if(!(of.pattrows=(UWORD*)MikMod_calloc(of.numpat+1,sizeof(UWORD)))) return 0;

	for(t=0;t<=of.numpat;t++) {
		of.pattrows[t]=64;
		for(s=0;s<of.numchn;s++)
		of.patterns[(t*of.numchn)+s]=tracks++;
	}

	return 1;
}

int AllocTracks(void)
{
	if(!of.numtrk) {
		_mm_errno=MMERR_NOT_A_MODULE;
		return 0;
	}
	if(!(of.tracks=(UBYTE **)MikMod_calloc(of.numtrk,sizeof(UBYTE *)))) return 0;
	return 1;
}

int AllocInstruments(void)
{
	int t,n;
	
	if(!of.numins) {
		_mm_errno=MMERR_NOT_A_MODULE;
		return 0;
	}
	if(!(of.instruments=(INSTRUMENT*)MikMod_calloc(of.numins,sizeof(INSTRUMENT))))
		return 0;

	for(t=0;t<of.numins;t++) {
		for(n=0;n<INSTNOTES;n++) { 
			/* Init note / sample lookup table */
			of.instruments[t].samplenote[n]   = n;
			of.instruments[t].samplenumber[n] = t;
		}   
		of.instruments[t].globvol = 64;
	}
	return 1;
}

int AllocSamples(void)
{
	UWORD u;

	if(!of.numsmp) {
		_mm_errno=MMERR_NOT_A_MODULE;
		return 0;
	}
	if(!(of.samples=(SAMPLE*)MikMod_calloc(of.numsmp,sizeof(SAMPLE)))) return 0;

	for(u=0;u<of.numsmp;u++) {
		of.samples[u].panning = 128; /* center */
		of.samples[u].handle  = -1;
		of.samples[u].globvol = 64;
		of.samples[u].volume  = 64;
	}
	return 1;
}

static int ML_LoadSamples(void)
{
	SAMPLE *s;
	int u;

	for(u=of.numsmp,s=of.samples;u;u--,s++)
		if(s->length) SL_RegisterSample(s,MD_MUSIC,modreader);

	return 1;
}

/* Creates a CSTR out of a character buffer of 'len' bytes, but strips any
   terminating non-printing characters like 0, spaces etc.                    */
CHAR *DupStr(CHAR* s,UWORD len,int strict)
{
	UWORD t;
	CHAR *d=NULL;

	/* Scan for last printing char in buffer [includes high ascii up to 254] */
	while(len) {
		if(s[len-1]>0x20) break;
		len--;
	}

	/* Scan forward for possible NULL character */
	if(strict) {
		for(t=0;t<len;t++) if (!s[t]) break;
		if (t<len) len=t;
	}

	/* When the buffer wasn't completely empty, allocate a cstring and copy the
	   buffer into that string, except for any control-chars */
	if((d=(CHAR*)MikMod_malloc(sizeof(CHAR)*(len+1)))) {
		for(t=0;t<len;t++) d[t]=(s[t]<32)?'.':s[t];
		d[len]=0;
	}
	return d;
}

CHAR *StrDup(CHAR *s)
{
	size_t l = strlen(s) + 1;
	CHAR *d = MikMod_malloc(l);
	strcpy(d, s);
	return d;
}

static void ML_XFreeSample(SAMPLE *s)
{
	if(s->handle>=0)
		MD_SampleUnload(s->handle);
	if(s->samplename) MikMod_free(s->samplename);
}

static void ML_XFreeInstrument(INSTRUMENT *i)
{
	if(i->insname) MikMod_free(i->insname);
}

static void ML_FreeEx(MODULE *mf)
{
	UWORD t;

	if(mf->songname) MikMod_free(mf->songname);
	if(mf->comment)  MikMod_free(mf->comment);

	if(mf->modtype)   MikMod_free(mf->modtype);
	if(mf->positions) MikMod_free(mf->positions);
	if(mf->patterns)  MikMod_free(mf->patterns);
	if(mf->pattrows)  MikMod_free(mf->pattrows);

	if(mf->tracks) {
		for(t=0;t<mf->numtrk;t++)
			if(mf->tracks[t]) MikMod_free(mf->tracks[t]);
		MikMod_free(mf->tracks);
	}
	if(mf->instruments) {
		for(t=0;t<mf->numins;t++)
			ML_XFreeInstrument(&mf->instruments[t]);
		MikMod_free(mf->instruments);
	}
	if(mf->samples) {
		for(t=0;t<mf->numsmp;t++)
			if(mf->samples[t].length) ML_XFreeSample(&mf->samples[t]);
		MikMod_free(mf->samples);
	}
	memset(mf,0,sizeof(MODULE));
	if(mf!=&of) MikMod_free(mf);
}

static MODULE *ML_AllocUniMod(void)
{
	MODULE *mf;

	return (mf=MikMod_malloc(sizeof(MODULE)));
}

static void Player_Free_internal(MODULE *mf)
{
	if(mf) {
		Player_Exit_internal(mf);
		ML_FreeEx(mf);
	}
}

MIKMODAPI void Player_Free(MODULE *mf)
{
	MUTEX_LOCK(vars);
	Player_Free_internal(mf);
	MUTEX_UNLOCK(vars);
}

static CHAR* Player_LoadTitle_internal(MREADER *reader)
{
	MLOADER *l;

	modreader=reader;
	_mm_errno = 0;
	_mm_critical = 0;
	_mm_iobase_setcur(modreader);

	/* Try to find a loader that recognizes the module */
	for(l=firstloader;l;l=l->next) {
		_mm_rewind(modreader);
		if(l->Test()) break;
	}

	if(!l) {
		_mm_errno = MMERR_NOT_A_MODULE;
		if(_mm_errorhandler) _mm_errorhandler();
		return NULL;
	}

	return l->LoadTitle();
}

MIKMODAPI CHAR* Player_LoadTitleFP(int fp)
{
	CHAR* result=NULL;
	MREADER* reader;

	if(fp && (reader=_mm_new_file_reader(fp))) {
		MUTEX_LOCK(lists);
		result=Player_LoadTitle_internal(reader);
		MUTEX_UNLOCK(lists);
		_mm_delete_file_reader(reader);
	}
	return result;
}

MIKMODAPI CHAR* Player_LoadTitleMem(const char *buffer,int len)
{
	CHAR *result=NULL;
	MREADER* reader;

	if ((reader=_mm_new_mem_reader(buffer,len)))
	{
		MUTEX_LOCK(lists);
		result=Player_LoadTitle_internal(reader);
		MUTEX_UNLOCK(lists);
		_mm_delete_mem_reader(reader);
	}
	
	
	return result;
}

MIKMODAPI CHAR* Player_LoadTitleGeneric(MREADER *reader)
{	
	CHAR *result=NULL;
	
	if (reader) {
		MUTEX_LOCK(lists);
		result=Player_LoadTitle_internal(reader);
		MUTEX_UNLOCK(lists);
	}
	return result;
}

MIKMODAPI CHAR* Player_LoadTitle(CHAR* filename)
{
	CHAR* result=NULL;
	int fp;
	MREADER* reader;

	if((fp=_mm_fopen(filename,"rb"))) {
		if((reader=_mm_new_file_reader(fp))) {
			MUTEX_LOCK(lists);
			result=Player_LoadTitle_internal(reader);
			MUTEX_UNLOCK(lists);
			_mm_delete_file_reader(reader);
		}
		_mm_fclose(fp);
	}
	return result;
}

/* Loads a module given an reader */
static MODULE* Player_LoadGeneric_internal(MREADER *reader,int maxchan,int curious)
{
	int t;
	MLOADER *l;
	int ok;
	MODULE *mf;

	modreader = reader;
	_mm_errno = 0;
	_mm_critical = 0;
	_mm_iobase_setcur(modreader);

	/* Try to find a loader that recognizes the module */
	for(l=firstloader;l;l=l->next) {
		_mm_rewind(modreader);
		if(l->Test()) break;
	}

	if(!l) {
		_mm_errno = MMERR_NOT_A_MODULE;
		if(_mm_errorhandler) _mm_errorhandler();
		_mm_rewind(modreader);_mm_iobase_revert(modreader);
		return NULL;
	}

	/* init unitrk routines */
	if(!UniInit()) {
		if(_mm_errorhandler) _mm_errorhandler();
		_mm_rewind(modreader);_mm_iobase_revert(modreader);
		return NULL;
	}

	/* init the module structure with vanilla settings */
	memset(&of,0,sizeof(MODULE));
	of.bpmlimit = 33;
	of.initvolume = 128;
	for (t = 0; t < UF_MAXCHAN; t++) of.chanvol[t] = 64;
	for (t = 0; t < UF_MAXCHAN; t++)
		of.panning[t] = ((t + 1) & 2) ? PAN_RIGHT : PAN_LEFT;

	/* init module loader and load the header / patterns */
	if (!l->Init || l->Init()) {
		_mm_rewind(modreader);
		ok = l->Load(curious);
		if (ok) {			
			/* propagate inflags=flags for in-module samples */
			for (t = 0; t < of.numsmp; t++)
				if (of.samples[t].inflags == 0)
					of.samples[t].inflags = of.samples[t].flags;
		}
	} else
		ok = 0;

	/* free loader and unitrk allocations */
	if (l->Cleanup) l->Cleanup();
	UniCleanup();

	if(!ok) {
		ML_FreeEx(&of);
		if(_mm_errorhandler) _mm_errorhandler();
		_mm_rewind(modreader);_mm_iobase_revert(modreader);
		return NULL;
	}

	if(!ML_LoadSamples()) {
		ML_FreeEx(&of);
		if(_mm_errorhandler) _mm_errorhandler();
		_mm_rewind(modreader);_mm_iobase_revert(modreader);
		return NULL;
	}

	if(!(mf=ML_AllocUniMod())) {
		ML_FreeEx(&of);
		_mm_rewind(modreader);_mm_iobase_revert(modreader);
		if(_mm_errorhandler) _mm_errorhandler();
		return NULL;
	}
	
	/* If the module doesn't have any specific panning, create a
	   MOD-like panning, with the channels half-separated. */
	if (!(of.flags & UF_PANNING))
		for (t = 0; t < of.numchn; t++)
			of.panning[t] = ((t + 1) & 2) ? PAN_HALFRIGHT : PAN_HALFLEFT;

	/* Copy the static MODULE contents into the dynamic MODULE struct. */
	memcpy(mf,&of,sizeof(MODULE));

	if(maxchan>0) {
		if(!(mf->flags&UF_NNA)&&(mf->numchn<maxchan))
			maxchan = mf->numchn;
		else
		  if((mf->numvoices)&&(mf->numvoices<maxchan))
			maxchan = mf->numvoices;

		if(maxchan<mf->numchn) mf->flags |= UF_NNA;

		if(MikMod_SetNumVoices_internal(maxchan,-1)) {
			_mm_iobase_revert(modreader);
			Player_Free(mf);
			return NULL;
		}
	}
	if(SL_LoadSamples()) {
		_mm_iobase_revert(modreader);
		Player_Free_internal(mf);
		return NULL;
	}
	if(Player_Init(mf)) {
		_mm_iobase_revert(modreader);
		Player_Free_internal(mf);
		mf=NULL;
	}
	_mm_iobase_revert(modreader);
	return mf;
}

MIKMODAPI MODULE* Player_LoadGeneric(MREADER *reader,int maxchan,int curious)
{
	MODULE* result;

	MUTEX_LOCK(vars);
	MUTEX_LOCK(lists);
		result=Player_LoadGeneric_internal(reader,maxchan,curious);
	MUTEX_UNLOCK(lists);
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI MODULE* Player_LoadMem(const char *buffer,int len,int maxchan,int curious)
{
	MODULE* result=NULL;
	MREADER* reader;

	if ((reader=_mm_new_mem_reader(buffer, len))) {
		result=Player_LoadGeneric(reader,maxchan,curious);
		_mm_delete_mem_reader(reader);
	}
	return result;
}

/* Loads a module given a file pointer.
   File is loaded from the current file seek position. */
MIKMODAPI MODULE* Player_LoadFP(int fp,int maxchan,int curious)
{
	MODULE* result=NULL;
	struct MREADER* reader=_mm_new_file_reader (fp);

	if (reader) {
		result=Player_LoadGeneric(reader,maxchan,curious);
		_mm_delete_file_reader(reader);
	}
	return result;
}

/* Open a module via its filename.  The loader will initialize the specified
   song-player 'player'. */
MIKMODAPI MODULE* Player_Load(CHAR* filename,int maxchan,int curious)
{
	int fp;
	MODULE *mf=NULL;

	if((fp=_mm_fopen(filename,"rb"))) {
		mf=Player_LoadFP(fp,maxchan,curious);
		_mm_fclose(fp);
	}
	return mf;
}

/* ex:set ts=4: */
