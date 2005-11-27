/* uclpack.c -- example program: a simple file packer

   This file is part of the UCL data compression library.

   Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The UCL library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The UCL library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the UCL library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
 */


/*************************************************************************
// NOTE: this is an example program, so do not use to backup your data
//
// This program lacks things like sophisticated file handling but is
// pretty complete regarding compression - it should provide a good
// starting point for adaption for you applications.
**************************************************************************/

#include <ucl/ucl.h>
#include "lutil.h"

static const char *progname = NULL;

static unsigned long total_in = 0;
static unsigned long total_out = 0;
static ucl_bool opt_debug = 0;

/* don't compute or verify checksum, always use fast decompressor */
static ucl_bool opt_fast = 0;

/* magic file header for compressed files */
static const unsigned char magic[8] =
    { 0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a };


/*************************************************************************
// file IO
**************************************************************************/

ucl_uint xread(FILE *f, ucl_voidp buf, ucl_uint len, ucl_bool allow_eof)
{
    ucl_uint l;

    l = ucl_fread(f,buf,len);
    if (l > len)
    {
        fprintf(stderr,"\nsomething's wrong with your C library !!!\n");
        exit(1);
    }
    if (l != len && !allow_eof)
    {
        fprintf(stderr,"\nread error - premature end of file\n");
        exit(1);
    }
    total_in += l;
    return l;
}

ucl_uint xwrite(FILE *f, const ucl_voidp buf, ucl_uint len)
{
    ucl_uint l;

    if (f != NULL)
    {
        l = ucl_fwrite(f,buf,len);
        if (l != len)
        {
            fprintf(stderr,"\nwrite error [%ld %ld]  (disk full ?)\n",
                   (long)len, (long)l);
            exit(1);
        }
    }
    total_out += len;
    return len;
}


int xgetc(FILE *f)
{
    unsigned char c;
    xread(f,(ucl_voidp) &c,1,0);
    return c;
}

void xputc(FILE *f, int c)
{
    unsigned char cc = (unsigned char) c;
    xwrite(f,(const ucl_voidp) &cc,1);
}

/* read and write portable 32-bit integers */

ucl_uint32 xread32(FILE *f)
{
    unsigned char b[4];
    ucl_uint32 v;

    xread(f,b,4,0);
    v  = (ucl_uint32) b[3] <<  0;
    v |= (ucl_uint32) b[2] <<  8;
    v |= (ucl_uint32) b[1] << 16;
    v |= (ucl_uint32) b[0] << 24;
    return v;
}

void xwrite32(FILE *f, ucl_uint32 v)
{
    unsigned char b[4];

    b[3] = (unsigned char) (v >>  0);
    b[2] = (unsigned char) (v >>  8);
    b[1] = (unsigned char) (v >> 16);
    b[0] = (unsigned char) (v >> 24);
    xwrite(f,b,4);
}


/*************************************************************************
// util
**************************************************************************/

static ucl_uint get_overhead(int method, ucl_uint size)
{
    if (method == 0x2b || method == 0x2d || method == 0x2e)
        return size / 8 + 256;
    return 0;
}


static char method_name[64];

static ucl_bool set_method_name(int method, int level)
{
    method_name[0] = 0;
    if (level < 1 || level > 10)
        return 0;
    if (method == 0x2b)
        sprintf(method_name,"NRV2B-99/%d", level);
    else if (method == 0x2d)
        sprintf(method_name,"NRV2D-99/%d", level);
    else if (method == 0x2e)
        sprintf(method_name,"NRV2E-99/%d", level);
    else
        return 0;
    return 1;
}


/*************************************************************************
// compress
**************************************************************************/

int do_compress(FILE *fi, FILE *fo, int method, int level, ucl_uint block_size)
{
    int r = 0;
    ucl_byte *in = NULL;
    ucl_byte *out = NULL;
    ucl_uint in_len;
    ucl_uint out_len;
    ucl_uint32 flags = opt_fast ? 0 : 1;
    ucl_uint32 checksum;
    ucl_uint overhead = 0;

    total_in = total_out = 0;

/*
 * Step 1: write magic header, flags & block size, init checksum
 */
    xwrite(fo,magic,sizeof(magic));
    xwrite32(fo,flags);
    xputc(fo,method);           /* compression method */
    xputc(fo,level);            /* compression level */
    xwrite32(fo,block_size);
    checksum = ucl_adler32(0,NULL,0);

/*
 * Step 2: allocate compression buffers and work-memory
 */
    overhead = get_overhead(method,block_size);
    in = (ucl_byte *) ucl_malloc(block_size);
    out = (ucl_byte *) ucl_malloc(block_size + overhead);
    if (in == NULL || out == NULL)
    {
        printf("%s: out of memory\n", progname);
        r = 1;
        goto err;
    }

/*
 * Step 3: process blocks
 */
    for (;;)
    {
        /* read block */
        in_len = xread(fi,in,block_size,1);
        if (in_len <= 0)
            break;

        /* update checksum */
        if (flags & 1)
            checksum = ucl_adler32(checksum,in,in_len);

        /* compress block */
        r = UCL_E_ERROR;
        if (method == 0x2b)
            r = ucl_nrv2b_99_compress(in,in_len,out,&out_len,0,level,NULL,NULL);
        else if (method == 0x2d)
            r = ucl_nrv2d_99_compress(in,in_len,out,&out_len,0,level,NULL,NULL);
        else if (method == 0x2e)
            r = ucl_nrv2e_99_compress(in,in_len,out,&out_len,0,level,NULL,NULL);
        if (r != UCL_E_OK || out_len > in_len + get_overhead(method,in_len))
        {
            /* this should NEVER happen */
            printf("internal error - compression failed: %d\n", r);
            r = 2;
            goto err;
        }

        /* write uncompressed block size */
        xwrite32(fo,in_len);

        if (out_len < in_len)
        {
            /* write compressed block */
            xwrite32(fo,out_len);
            xwrite(fo,out,out_len);
        }
        else
        {
            /* not compressible - write uncompressed block */
            xwrite32(fo,in_len);
            xwrite(fo,in,in_len);
        }
    }

    /* write EOF marker */
    xwrite32(fo,0);

    /* write checksum */
    if (flags & 1)
        xwrite32(fo,checksum);

    r = 0;
err:
    ucl_free(out);
    ucl_free(in);
    return r;
}


/*************************************************************************
// decompress / test
//
// We are using overlapping (in-place) decompression to save some
// memory - see overlap.c.
**************************************************************************/

int do_decompress(FILE *fi, FILE *fo)
{
    int r = 0;
    ucl_byte *buf = NULL;
    ucl_uint buf_len;
    unsigned char m [ sizeof(magic) ];
    ucl_uint32 flags;
    int method;
    int level;
    ucl_uint block_size;
    ucl_uint32 checksum;
    ucl_uint overhead = 0;

    total_in = total_out = 0;

/*
 * Step 1: check magic header, read flags & block size, init checksum
 */
    if (xread(fi,m,sizeof(magic),1) != sizeof(magic) ||
        memcmp(m,magic,sizeof(magic)) != 0)
    {
        printf("%s: header error - this file is not compressed by uclpack\n", progname);
        r = 1;
        goto err;
    }
    flags = xread32(fi);
    method = xgetc(fi);
    level = xgetc(fi);
    block_size = xread32(fi);
    overhead = get_overhead(method,block_size);
    if (overhead == 0 || !set_method_name(method, level))
    {
        printf("%s: header error - invalid method %d (level %d)\n",
                progname, method, level);
        r = 2;
        goto err;
    }
    if (block_size < 1024 || block_size > 8*1024*1024L)
    {
        printf("%s: header error - invalid block size %ld\n",
                progname, (long) block_size);
        r = 3;
        goto err;
    }
    checksum = ucl_adler32(0,NULL,0);

/*
 * Step 2: allocate buffer for in-place decompression
 */
    buf_len = block_size + overhead;
    buf = (ucl_byte *) ucl_malloc(buf_len);
    if (buf == NULL)
    {
        printf("%s: out of memory\n", progname);
        r = 4;
        goto err;
    }

/*
 * Step 3: process blocks
 */
    for (;;)
    {
        ucl_byte *in;
        ucl_byte *out;
        ucl_uint in_len;
        ucl_uint out_len;

        /* read uncompressed size */
        out_len = xread32(fi);

        /* exit if last block (EOF marker) */
        if (out_len == 0)
            break;

        /* read compressed size */
        in_len = xread32(fi);

        /* sanity check of the size values */
        if (in_len > block_size || out_len > block_size ||
            in_len == 0 || in_len > out_len)
        {
            printf("%s: block size error - data corrupted\n", progname);
            r = 5;
            goto err;
        }

        /* place compressed block at the top of the buffer */
        in = buf + buf_len - in_len;
        out = buf;

        /* read compressed block data */
        xread(fi,in,in_len,0);

        if (in_len < out_len)
        {
            /* decompress - use safe decompressor as data might be corrupted */
            ucl_uint new_len = out_len;

            if (method == 0x2b)
            {
                if (opt_fast)
                    r = ucl_nrv2b_decompress_8(in,in_len,out,&new_len,NULL);
                else
                    r = ucl_nrv2b_decompress_safe_8(in,in_len,out,&new_len,NULL);
            }
            else if (method == 0x2d)
            {
                if (opt_fast)
                    r = ucl_nrv2d_decompress_8(in,in_len,out,&new_len,NULL);
                else
                    r = ucl_nrv2d_decompress_safe_8(in,in_len,out,&new_len,NULL);
            }
            else if (method == 0x2e)
            {
                if (opt_fast)
                    r = ucl_nrv2e_decompress_8(in,in_len,out,&new_len,NULL);
                else
                    r = ucl_nrv2e_decompress_safe_8(in,in_len,out,&new_len,NULL);
            }
            if (r != UCL_E_OK || new_len != out_len)
            {
                printf("%s: compressed data violation: error %d (0x%x: %ld/%ld/%ld)\n", progname, r, method, (long) in_len, (long) out_len, (long) new_len);
                r = 6;
                goto err;
            }
            /* write decompressed block */
            xwrite(fo,out,out_len);
            /* update checksum */
            if ((flags & 1) && !opt_fast)
                checksum = ucl_adler32(checksum,out,out_len);
        }
        else
        {
            /* write original (incompressible) block */
            xwrite(fo,in,in_len);
            /* update checksum */
            if ((flags & 1) && !opt_fast)
                checksum = ucl_adler32(checksum,in,in_len);
        }
    }

    /* read and verify checksum */
    if (flags & 1)
    {
        ucl_uint32 c = xread32(fi);
        if (!opt_fast && c != checksum)
        {
            printf("%s: checksum error - data corrupted\n", progname);
            r = 7;
            goto err;
        }
    }

    r = 0;
err:
    ucl_free(buf);
    return r;
}


/*************************************************************************
//
**************************************************************************/

static void usage(void)
{
    printf("usage: %s [-123456789] input-file output-file   (compress)\n", progname);
    printf("usage: %s -d input-file output-file             (decompress)\n", progname);
    printf("usage: %s -t input-file...                      (test)\n", progname);
    exit(1);
}


/* open input file */
static FILE *xopen_fi(const char *name)
{
    FILE *f;

    f = fopen(name,"rb");
    if (f == NULL)
    {
        printf("%s: cannot open input file %s\n", progname, name);
        exit(1);
    }
#if defined(HAVE_STAT) && defined(S_ISREG)
    {
        struct stat st;
#if defined(HAVE_LSTAT)
        if (lstat(name,&st) != 0 || !S_ISREG(st.st_mode))
#else
        if (stat(name,&st) != 0 || !S_ISREG(st.st_mode))
#endif
        {
            printf("%s: %s is not a regular file\n", progname, name);
            fclose(f);
            exit(1);
        }
    }
#endif
    return f;
}


/* open output file */
static FILE *xopen_fo(const char *name)
{
    FILE *f;

#if 0
    /* this is an example program, so make sure we don't overwrite a file */
    f = fopen(name,"rb");
    if (f != NULL)
    {
        printf("%s: file %s already exists -- not overwritten\n", progname, name);
        fclose(f);
        exit(1);
    }
#endif
    f = fopen(name,"wb");
    if (f == NULL)
    {
        printf("%s: cannot open output file %s\n", progname, name);
        exit(1);
    }
    return f;
}


/*************************************************************************
//
**************************************************************************/

int main(int argc, char *argv[])
{
    int i = 1;
    int r = 0;
    FILE *fi = NULL;
    FILE *fo = NULL;
    const char *in_name = NULL;
    const char *out_name = NULL;
    ucl_bool opt_decompress = 0;
    ucl_bool opt_test = 0;
    int opt_method = 0x2b;
    int opt_level = 7;
#if defined(MAINT)
    ucl_uint opt_block_size = (2*1024*1024L);
#else
    ucl_uint opt_block_size = (256*1024L);
#endif
    const char *s;

#if defined(__EMX__)
    _response(&argc,&argv);
    _wildcard(&argc,&argv);
#endif
    progname = argv[0];
    for (s = progname; *s; s++)
        if (*s == '/' || *s == '\\')
            progname = s + 1;

    printf("\nUCL real-time data compression library (v%s, %s).\n",
            ucl_version_string(), ucl_version_date());
    printf("Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer\n\n");

    printf(
"*** WARNING ***\n"
"   This is an example program, do not use to backup your data !\n"
"\n");

/*
 * Step 1: initialize the UCL library
 */
    if (ucl_init() != UCL_E_OK)
    {
        printf("ucl_init() failed !!!\n");
        exit(1);
    }

/*
 * Step 2: get options
 */

    while (i < argc && argv[i][0] == '-')
    {
        if (strcmp(argv[i],"-d") == 0)
            opt_decompress = 1;
        else if (strcmp(argv[i],"-t") == 0)
            opt_test = 1;
        else if (strcmp(argv[i],"-F") == 0)
            opt_fast = 1;
        else if (strcmp(argv[i],"--2b") == 0)
            opt_method = 0x2b;
        else if (strcmp(argv[i],"--nrv2b") == 0)
            opt_method = 0x2b;
        else if (strcmp(argv[i],"--2d") == 0)
            opt_method = 0x2d;
        else if (strcmp(argv[i],"--nrv2d") == 0)
            opt_method = 0x2d;
        else if (strcmp(argv[i],"--2e") == 0)
            opt_method = 0x2e;
        else if (strcmp(argv[i],"--nrv2e") == 0)
            opt_method = 0x2e;
        else if ((argv[i][1] >= '1' && argv[i][1] <= '9') && !argv[i][2])
            opt_level = argv[i][1] - '0';
        else if (strcmp(argv[i],"--10") == 0)
            opt_level = 10;
        else if (strcmp(argv[i],"--best") == 0)
            opt_level = 10;
        else if (argv[i][1] == 'b' && argv[i][2])
        {
#if (UCL_UINT_MAX > UINT_MAX) && defined(HAVE_ATOL)
            ucl_int b = (ucl_int) atol(&argv[i][2]);
#else
            ucl_int b = (ucl_int) atoi(&argv[i][2]);
#endif
            if (b >= 1024L && b <= 8*1024*1024L)
                opt_block_size = b;
        }
        else if (strcmp(argv[i],"--debug") == 0)
            opt_debug = 1;
        else
            usage();
        i++;
    }
    if (opt_test && i >= argc)
        usage();
    if (!opt_test && i + 2 != argc)
        usage();

/*
 * Step 3: process file(s)
 */
    if (opt_test)
    {
        while (i < argc && r == 0)
        {
            in_name = argv[i++];
            fi = xopen_fi(in_name);
            r = do_decompress(fi,NULL);
            if (r == 0)
                printf("%s: tested ok: %-10s %-11s: %6ld -> %6ld bytes\n",
                        progname, in_name, method_name, total_in, total_out);
            fclose(fi);
            fi = NULL;
        }
    }
    else if (opt_decompress)
    {
        in_name = argv[i++];
        out_name = argv[i++];
        fi = xopen_fi(in_name);
        fo = xopen_fo(out_name);
        r = do_decompress(fi,fo);
        if (r == 0)
            printf("%s: decompressed %ld into %ld bytes\n",
                    progname, total_in, total_out);
    }
    else /* compress */
    {
        if (!set_method_name(opt_method, opt_level))
        {
            printf("%s: internal error - invalid method %d (level %d)\n",
                   progname, opt_method, opt_level);
            goto quit;
        }
        in_name = argv[i++];
        out_name = argv[i++];
        fi = xopen_fi(in_name);
        fo = xopen_fo(out_name);
        r = do_compress(fi,fo,opt_method,opt_level,opt_block_size);
        if (r == 0)
            printf("%s: algorithm %s, compressed %ld into %ld bytes\n",
                    progname, method_name, total_in, total_out);
    }

quit:
    if (fi) fclose(fi);
    if (fo) fclose(fo);
    return r;
}

/*
vi:ts=4:et
*/

