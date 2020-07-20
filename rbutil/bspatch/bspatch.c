/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef WIN32
#include <io.h>
#else
#include <stdarg.h>
#include <sys/types.h>
#endif
#include "../bzip2/bzlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define errx err
void err(int exitcode, const char * fmt, ...)
{
	va_list valist;
	va_start(valist, fmt);
	vprintf(fmt, valist);
	va_end(valist);
	exit(exitcode);
}

static long offtin(u_char *buf)
{
	long y;

	y = buf[7] & 0x7F;
	y = y * 256;y += buf[6];
	y = y * 256;y += buf[5];
	y = y * 256;y += buf[4];
	y = y * 256;y += buf[3];
	y = y * 256;y += buf[2];
	y = y * 256;y += buf[1];
	y = y * 256;y += buf[0];

	if (buf[7] & 0x80) y = -y;

	return y;
}

int apply_bspatch(const char *infile, const char *outfile, const char *patchfile)
{
	FILE * f, *cpf, *dpf, *epf;
	BZFILE * cpfbz2, *dpfbz2, *epfbz2;
	int cbz2err, dbz2err, ebz2err;
	FILE * fs;
	long oldsize, newsize;
	long bzctrllen, bzdatalen;
	u_char header[32], buf[8];
	u_char *pold, *pnew;
	long oldpos, newpos;
	long ctrl[3];
	long lenread;
	long i;

	/* Open patch file */
	if ((f = fopen(patchfile, "r")) == NULL)
		err(1, "fopen(%s)", patchfile);

	/*
	File format:
		0	8	"BSDIFF40"
		8	8	X
		16	8	Y
		24	8	sizeof(newfile)
		32	X	bzip2(control block)
		32+X	Y	bzip2(diff block)
		32+X+Y	???	bzip2(extra block)
	with control block a set of triples (x,y,z) meaning "add x bytes
	from oldfile to x bytes from the diff block; copy y bytes from the
	extra block; seek forwards in oldfile by z bytes".
	*/

	/* Read header */
	if (fread(header, 1, 32, f) < 32) {
		if (feof(f))
			errx(1, "Corrupt patch\n");
		err(1, "fread(%s)", patchfile);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "BSDIFF40", 8) != 0)
		errx(1, "Corrupt patch\n");

	/* Read lengths from header */
	bzctrllen = offtin(header + 8);
	bzdatalen = offtin(header + 16);
	newsize = offtin(header + 24);
	if ((bzctrllen < 0) || (bzdatalen < 0) || (newsize < 0))
		errx(1, "Corrupt patch\n");

	/* Close patch file and re-open it via libbzip2 at the right places */
	if (fclose(f))
		err(1, "fclose(%s)", patchfile);
	if ((cpf = fopen(patchfile, "rb")) == NULL)
		err(1, "fopen(%s)", patchfile);
	if (fseek(cpf, 32, SEEK_SET))
		err(1, "fseeko(%s, %lld)", patchfile,
		(long long)32);
	if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL)
		errx(1, "BZ2_bzReadOpen, bz2err = %d", cbz2err);
	if ((dpf = fopen(patchfile, "rb")) == NULL)
		err(1, "fopen(%s)", patchfile);
	if (fseek(dpf, 32 + bzctrllen, SEEK_SET))
		err(1, "fseeko(%s, %lld)", patchfile,
		(long long)(32 + bzctrllen));
	if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL)
		errx(1, "BZ2_bzReadOpen, bz2err = %d", dbz2err);
	if ((epf = fopen(patchfile, "rb")) == NULL)
		err(1, "fopen(%s)", patchfile);
	if (fseek(epf, 32 + bzctrllen + bzdatalen, SEEK_SET))
		err(1, "fseeko(%s, %lld)", patchfile,
		(long long)(32 + bzctrllen + bzdatalen));
	if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL)
		errx(1, "BZ2_bzReadOpen, bz2err = %d", ebz2err);

	fs = fopen(infile, "rb");
	if (fs == NULL)err(1, "Open failed :%s", infile);
	if (fseek(fs, 0, SEEK_END) != 0)err(1, "Seek failed :%s", infile);
	oldsize = ftell(fs);
	pold = (u_char *)malloc(oldsize + 1);
	if (pold == NULL)	err(1, "Malloc failed :%s", infile);
	fseek(fs, 0, SEEK_SET);
	if (fread(pold, 1, oldsize, fs) == -1)	err(1, "Read failed :%s", infile);
	if (fclose(fs) == -1)	err(1, "Close failed :%s", infile);

	pnew = malloc(newsize + 1);
	if (pnew == NULL)err(1, NULL);

	oldpos = 0;newpos = 0;
	while (newpos < newsize) {
		/* Read control data */
		for (i = 0;i <= 2;i++) {
			lenread = BZ2_bzRead(&cbz2err, cpfbz2, buf, 8);
			if ((lenread < 8) || ((cbz2err != BZ_OK) &&
				(cbz2err != BZ_STREAM_END)))
				errx(1, "Corrupt patch\n");
			ctrl[i] = offtin(buf);
		};

		/* Sanity-check */
		if (newpos + ctrl[0] > newsize)
			errx(1, "Corrupt patch\n");

		/* Read diff string */
		lenread = BZ2_bzRead(&dbz2err, dpfbz2, pnew + newpos, ctrl[0]);
		if ((lenread < ctrl[0]) ||
			((dbz2err != BZ_OK) && (dbz2err != BZ_STREAM_END)))
			errx(1, "Corrupt patch\n");

		/* Add pold data to diff string */
		for (i = 0;i < ctrl[0];i++)
			if ((oldpos + i >= 0) && (oldpos + i < oldsize))
				pnew[newpos + i] += pold[oldpos + i];

		/* Adjust pointers */
		newpos += ctrl[0];
		oldpos += ctrl[0];

		/* Sanity-check */
		if (newpos + ctrl[1] > newsize)
			errx(1, "Corrupt patch\n");

		/* Read extra string */
		lenread = BZ2_bzRead(&ebz2err, epfbz2, pnew + newpos, ctrl[1]);
		if ((lenread < ctrl[1]) ||
			((ebz2err != BZ_OK) && (ebz2err != BZ_STREAM_END)))
			errx(1, "Corrupt patch\n");

		/* Adjust pointers */
		newpos += ctrl[1];
		oldpos += ctrl[2];
	};

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&cbz2err, cpfbz2);
	BZ2_bzReadClose(&dbz2err, dpfbz2);
	BZ2_bzReadClose(&ebz2err, epfbz2);
	if (fclose(cpf) || fclose(dpf) || fclose(epf))
		err(1, "fclose(%s)", patchfile);

	/* Write the pnew file */
	fs = fopen(outfile, "wb");
	if (fs == NULL)err(1, "Create failed :%s", outfile);
	if (fwrite(pnew, 1, newsize, fs) == -1)err(1, "Write failed :%s", outfile);
	if (fclose(fs) == -1)err(1, "Close failed :%s", outfile);

	free(pnew);
	free(pold);

	return 0;
}
