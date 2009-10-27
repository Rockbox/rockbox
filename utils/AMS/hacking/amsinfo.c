/*
 * Copyright © 2008 Rafaël Carré <rafael.carre@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 *
 */

#define _ISOC99_SOURCE /* snprintf() */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#if 1 /* ANSI colors */

#	define color(a) printf("%s",a)
char OFF[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

char GREY[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
char RED[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
char GREEN[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
char YELLOW[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
char BLUE[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

#else
	/* disable colors */
#	define color(a)
#endif

#define LIB_OFFSET 160 /* FIXME (see below) */
/* The alignement of library blocks (in number of 0x200 bytes blocks)
 * alignement	-	md5sum		-			filename		-	model
 * 120 : fc9dd6116001b3e6a150b898f1b091f0  m200p-4.1.08A.bin	M200
 * 128 : 82e3194310d1514e3bbcd06e84c4add3  m200p.bin			Fuze
 * 160 : c12711342169c66e209540cd1f27cd26  m300f.bin			CLIP
 *
 * Note : the size of library blocks is variable:
 *
 * For m200p-4.1.08A.bin it's always 0x1e000 blocks = 240 * 0x200
 *
 * For m200p.bin it can be 0x20000 (256*0x200) or 0x40000 (512*0x200)
 * 		(for "acp_decoder" and "sd_reload__" blocks)
 *
 * For m300f.bin it can be 0x28000 (320*0x200) or 0x14000 (160 * 0x200)
 *
 */

#define bug(...) do { fprintf(stderr,"ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

/* byte swapping */
#define get32le(a) ((uint32_t) \
    ( buf[a+3] << 24 | buf[a+2] << 16 | buf[a+1] << 8 | buf[a] ))
#define get16le(a) ((uint16_t)( buf[a+1] << 8 | buf[a] ))

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

/* globals */

size_t sz;	/* file size */
uint8_t *buf; /* file content */


/* 1st block description */
uint32_t idx,checksum,bs_multiplier,firmware_sz;
uint32_t unknown_4_1; uint8_t unknown_1,id; uint16_t unknown_2;
uint32_t unknown_4_2,unknown_4_3;

static void *xmalloc(size_t s) /* malloc helper */
{
	void * r = malloc(s);
	if(!r) bugp("malloc");
	return r;
}

/* known models */
static const char * model(uint8_t id)
{
	switch(id)
	{
		case 0x1E: return "FUZE"; break;
		case 0x22: return "CLIP"; break;
		case 0x23: return "C200"; break;
		case 0x24: return "E200"; break;
		case 0x25: return "M200"; break;
		case 0x27: return "CLV2"; break;
        case 0x70:
		case 0x6d: return "FUZ2"; break;
		default:
		printf("Unknown ID 0x%x\n", id);

		assert(id == 0x1E || (id > 0x21 && id < 0x26));
		return "UNKNOWN!";
	}
}

/* checksums the firmware (the firmware header contains the verification) */
static uint32_t do_checksum(void)
{
	uint32_t c = 0;

	size_t i = 0x400/4;
	while(i<(0x400+firmware_sz)/4)
		c += ((uint32_t*)buf)[i++];

	return c;
}

/* verify the firmware header */
static void check(void)
{
	uint32_t checksum2;

	assert(sz >= 0x400 && sz % 0x200 == 0);

	size_t i;
	checksum2 = 0;
	for(i=0;i<sz/4-1;i++)
		checksum2 += ((uint32_t*)buf)[i];

	uint32_t last_word = get32le(sz - 4);

	switch(last_word)
	{
		case 0:				/* no whole file checksum */
			break;
		case 0xefbeadde:	/* no whole file checksum */
			break;
		default:			/* verify whole file checksum */
			assert(last_word == checksum2);
	}

	idx = get32le(0);
	unsigned int shift = (get32le(4) == 0x0000f000) ? 4 : 0;
	checksum = get32le(4 + shift);
	bs_multiplier = get32le(8 + shift);
	firmware_sz = get32le(0xc + shift);
	assert(bs_multiplier << 9 == PAD_TO_BOUNDARY(firmware_sz)); /* 0x200 * bs_multiplier */

	unknown_4_1 = get32le(0x10 + shift);
	unknown_1 = buf[0x14 + shift];
	id = buf[0x15 + shift];
	unknown_2 = get16le(0x16 + shift);
	unknown_4_2 = get32le(0x18 + shift);
	unknown_4_3 = get32le(0x1c + shift);

	color(GREEN);
	printf("4 Index                  %d\n",idx);
	assert(idx == 0);
	color(GREEN);
	printf("4 Firmware Checksum      %x",checksum);
	checksum2=do_checksum();
	color(GREEN);
	printf(" (%x)\n",checksum2);
	assert(checksum == checksum2);
	color(GREEN);
	printf("4 Block Size Multiplier  %x\n",bs_multiplier);
	color(GREEN);
	printf("4 Firmware block size    %x (%d)\n",firmware_sz,firmware_sz);

	color(GREEN);
	printf("4 Unknown (should be 3)  %x\n",unknown_4_1);
	assert(unknown_4_1 == 3);

	/* variable */
	color(GREEN);
	printf("1 Unknown                %x\n",unknown_1);

	color(GREEN);
	printf("1 Model ID               %x (%s)\n",id,model(id));

	color(GREEN);
	printf("2 Unknown (should be 0)  %x\n",unknown_2);
	assert(unknown_2 == 0);

	color(GREEN);
	printf("4 Unknown (should be 40) %x\n",unknown_4_2);
	assert(unknown_4_2 == 0x40 );

	color(GREEN);
	printf("4 Unknown (should be 1)  %x\n",unknown_4_3);
	assert(unknown_4_3 == 1);

	/* rest of the block is padded with 0xff */
	for(i=0x20 + shift;i<0x200 - shift;i++)
		assert(buf[i]==0xff /* normal case */ ||
			((id==0x1e||id==0x24) && ( /* Fuze or E200 */
				(i>=0x3c && i<=0x3f && get32le(0x3c)==0x00005000)
			)));

	/* the 2nd block is identical, except that the 1st byte has been incremented */
	assert(buf[0x0]==0&&buf[0x200]==1);
	assert(!memcmp(&buf[1],&buf[0x201],0x1FF - shift));
}

typedef enum
{
#if 0
	FW_HEADER,
	FW,
#endif
	LIB,
	PAD,
	HEADER,
	UNKNOWN
} type;

static unsigned int n_libs = 0, n_pads_ff = 0, n_pads_deadbeef = 0, n_unkn = 0, n_headers = 0;

static void show_lib(size_t off)
{
	/* first word: char* */
	uint32_t start = get32le(off+4);
	uint32_t stop = get32le(off+8);

	uint32_t size = get32le(off+0xc);

#if 0 /* library block hacking */
	/* assert(stop > start); */

	/* assert(stop - start == size); */

	if(stop - start != size)
	{
		color(RED);
		printf("STOP - START != SIZE || 0x%.8x - 0x%.8x == 0x%.8x != 0x%.8x\n",
				stop, start, stop - start, size);
	}

	color(RED);
	printf("0x%.8x -> 0x%.8x SIZE 0x%.6x\n", start, stop, size);

	uint32_t first = get32le(off+0x10); /* ? */
	printf("? = 0x%.8x , ",first);
#endif

	uint32_t funcs = get32le(off+0x14); /* nmbr of functions */
	color(YELLOW);
	printf("\t%d funcs",funcs);

	unsigned int i;
	for(i=0;i<funcs;i++)
	{
		uint32_t fptr = get32le(off+0x18+i*4);
		if(!fptr)
		{
			assert(funcs==1); /* if 1 function is exported, it's empty */
		}
		else
		{
			assert(fptr - start < 0x0000ffff);
			/* printf("0x%.4x ",fptr); */
		}
	}

	color(BLUE);
	printf("\tBASE 0x%.8x (code + 0x%x) END 0x%.8x : SIZE 0x%.8x\n",start, 0x18 + i*4, stop, stop - start);

	char name[12+1];
	memcpy(name,&buf[off+get32le(off)],12);
	name[12] = '\0';

	FILE *out = fopen(name,"w");
	if(!out)
		bug("library block");

	if(fwrite(&buf[off],size,1,out)!=1)
		bug();

	fclose(out);
}

static int unknown = 0;
static int padding = 0;
static void print_block(size_t off, type t)
{
	/* reset counters if needed */
	if(t != UNKNOWN && unknown)
	{	/* print only the number of following blocks */
		color(GREY);
		printf("%d unknown blocks (0x%.6x bytes)\n",unknown,unknown*0x200);
		unknown = 0;
	}
	else if(t != PAD && padding)
	{	/* same */
		color(GREY);
		printf("%d padding blocks (0x%.6x bytes)\n",padding,padding*0x200);
		padding = 0;
	}

	if(t != UNKNOWN && t != PAD) /* for other block types, always print the offset */
	{
		color(GREEN);
		printf("0x%.6x\t", (unsigned int)off);
		color(OFF);
	}

	switch(t)
	{
		size_t s;
		FILE *f;
		char filename[8+4]; /* unknown\0 , 10K max */
#if 0
		case FW_HEADER:
			printf("firmware header	0x%x\n",off);
			break;
		case FW:
			printf("firmware block	0x%x\n",off);
			break;
#endif
		case LIB:
			s = LIB_OFFSET * 0x200;
			while(s < get32le(off+12))
				s <<= 1;
			color(RED);
			printf("library block	0x%.6x\t->\t0x%.6x\t\"%s\"\n",
					(unsigned int)s, (unsigned int)(off+s),
                    &buf[off+get32le(off)]);
			show_lib(off);
			n_libs++;
			break;
		case PAD:
			if(buf[off] == 0xff)
				n_pads_ff++;
			else
				n_pads_deadbeef++;
			padding++;
			break;
		case UNKNOWN:
			unknown++;
            n_unkn++;
#if 0       /* do not dump unknown blocks */
			snprintf(filename, sizeof(filename), "unknown%d", n_unkn);
			f = fopen(filename, "w");
			if(f)
			{
				if( fwrite(buf+off, 0x200, 1, f) != 1 )
					bugp("unknown block");
				fclose(f);
			}
			else
				bugp("unknown block");
#endif
			break;
		case HEADER:
			color(YELLOW);
			printf("header block	0x%.6x\t->\t0x%.6x\n",
					PAD_TO_BOUNDARY(get32le(off)),
					(unsigned int)PAD_TO_BOUNDARY(off+get32le(off)));
	        snprintf(filename, sizeof(filename), "header%d", n_headers++);
	        f = fopen(filename,"w");
	        if(!f)
	            bug("header");

	        if(fwrite(&buf[off],get32le(off),1,f)!=1)
	            bug();

	        fclose(f);

			break;
		default:
			abort();
	}

	if(t != PAD && t != UNKNOWN)
		printf("\n");
}

static size_t verify_block(size_t off)
{
	assert(!(off%0x200));
	assert(off+0x200 < sz);

	size_t s = 0x200;
	type t = UNKNOWN;

	size_t offset_str = get32le(off);
	if(get32le(off) == 0xefbeadde )
	{
#if 0 /* some blocks begin with 0xdeadbeef but aren't padded with that value */
		unsigned int i;
		for(i=0;i<s;i+=4)
			assert(get32le(off+i) == 0xefbeadde);
#endif
		t = PAD;
	}
	else if( *(uint32_t*)(&buf[off]) == 0xffffffff)
	{
		unsigned int i;
		for(i=0;i<s;i++)
			assert(buf[off+i] == 0xff);
		t = PAD;
	}
	else if(off+offset_str+12<sz) /* XXX: we should check that the address at which
									* the string is located is included in this
									* library block's size, but we only know the
									* block's size after we confirmed that this is 
									* a library block (by looking at the 11 chars
									* ASCII string). */
	{
		short int ok = 1;
		unsigned int i;
		for(i=0;i<11;i++)
			if(buf[off+offset_str+i] >> 7 || !buf[off+offset_str+i])
				ok = 0;
		if(buf[off+offset_str+11])
			ok = 0;
		if(ok) /* library block */
		{
			t = LIB;
			s = LIB_OFFSET * 0x200;
			while(s < get32le(off+12)) 	/* of course the minimum is the size
												* specified in the block header */
				s <<= 1;
		}
		else
			t = UNKNOWN;
	}
	else
		t = UNKNOWN;

	if(t==UNKNOWN)
	{
		if(!strncmp((char*)buf+off+8,"HEADER",6))
		{
			s = PAD_TO_BOUNDARY(get32le(off)); /* first 4 bytes le are the block size */
			t = HEADER;
		}
	}

	print_block(off,t);

	return PAD_TO_BOUNDARY(s);
}

static void extract(void)
{
	FILE *out = fopen("firmware","w");
	if(!out)
		bug("firmware");

	if(fwrite(&buf[0x400],firmware_sz,1,out)!=1)
		bug("firmare writing");
	fclose(out);

	off_t off = PAD_TO_BOUNDARY(0x400 + firmware_sz);
	unsigned int n = 0;

	printf("\n");
	color(RED);
	printf("Extracting\n\n");

	while((unsigned int)(off+0x200)<sz)
	{
		/* look at the next 0x200 bytes if we can recognize a block type */
		off += verify_block(off); /* then skip its real size */
		n++;	/* and look at the next block ;) */
	}

	/* statistics */
	printf("\n");
	color(RED);
	printf("TOTAL\t%d\tblocks (%d unknown)\n",n,n_unkn);
	color(BLUE);
	printf("\t%d\tlibs\n",n_libs);
	color(GREY);
	printf("\t%d\tpads ff\n",n_pads_ff);
	color(GREY);
	printf("\t%d\tpads deadbeef\n",n_pads_deadbeef);
	color(GREEN);
	printf("\t%d\theaders\n",n_headers);
}

int main(int argc, const char **argv)
{
	int fd;
	struct stat st;
	if(argc != 2)
		bug("Usage: %s <firmware>\n",*argv);

	if( (fd = open(argv[1],O_RDONLY)) == -1 )
		bugp("opening firmware failed");

	if(fstat(fd,&st) == -1)
		bugp("firmware stat() failed");
	sz = st.st_size;

	buf=xmalloc(sz);
	if(read(fd,buf,sz)!=(ssize_t)sz) /* load the whole file into memory */
		bugp("reading firmware");

	close(fd);

	check();	/* verify header and checksums */
	extract();	/* split in blocks */

	free(buf);
	return 0;
}
