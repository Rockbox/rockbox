#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ucl/ucl.h>

int level = 10;
int method = 0x2e;

void write_be32(uint8_t* buf, uint32_t x)
{
    buf[0] = (x >> 24) & 0xff;
    buf[1] = (x >> 16) & 0xff;
    buf[2] = (x >>  8) & 0xff;
    buf[3] = (x >>  0) & 0xff;
}

uint8_t* uclpack(const uint8_t* buf, size_t size, size_t* outsize)
{
    /* The following formula comes from the UCL documentation */
    size_t max_size = size + (size / 8) + 256;

    /* Allocate output buffer */
    uint8_t* outbuf = malloc(max_size);
    if(!outbuf) {
        fprintf(stderr, "Out of memory\n");
        return NULL;
    }

    /* Compress entire image as one block */
    ucl_uint outsize_ucl;
    int rc;
    switch(method) {
    case 0x2b:
        rc = ucl_nrv2b_99_compress((const ucl_bytep)buf, size,
                                   (ucl_bytep)outbuf, &outsize_ucl,
                                   NULL, level, NULL, NULL);
        break;
    case 0x2d:
        rc = ucl_nrv2d_99_compress((const ucl_bytep)buf, size,
                                   (ucl_bytep)outbuf, &outsize_ucl,
                                   NULL, level, NULL, NULL);
        break;
    case 0x2e:
    default:
        rc = ucl_nrv2e_99_compress((const ucl_bytep)buf, size,
                                   (ucl_bytep)outbuf, &outsize_ucl,
                                   NULL, level, NULL, NULL);
        break;
    }

    if(rc != UCL_E_OK || outsize_ucl > max_size) {
        free(outbuf);
        fprintf(stderr, "UCL compression failed (%d)\n", rc);
        return NULL;
    }

    *outsize = outsize_ucl;
    return outbuf;
}

uint8_t* load_file(const char* filename, size_t* outsize)
{
    uint8_t* buf = NULL;
    FILE* f = fopen(filename, "rb");
    if(!f) {
        fprintf(stderr, "cannot open input file: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(size < 0) {
        fprintf(stderr, "seek error in file: %s\n", filename);
        goto out;
    } else if(size == 0) {
        static uint8_t nul;
        buf = &nul;
        goto out;
    }

    buf = malloc(size);
    if(!buf) {
        fprintf(stderr, "out of memory\n");
        goto out;
    }

    if(fread(buf, size, 1, f) != 1) {
        fprintf(stderr, "error reading input file: %s\n", filename);
        free(buf);
        buf = NULL;
        goto out;
    }

  out:
    fclose(f);
    if(buf)
        *outsize = size;
    return buf;
}

void usage(void)
{
    printf(
"Usage:\n"
"  makeucl [--nrv2b|--nrv2d|--nrv2e] [--level 1-10] infile outfile\n"
"\n"
"Methods supported: nrv2b, nrv2d, nrv2e (default)\n"
"Compression level: 1 (lowest) to 10 (highest, default)\n"
"\n"
"This tool is similar to uclpack, but generates a single compressed stream\n"
"to simplify decompression at the cost of worse overall compression ratios.\n"
"Most suitable for applications that are code-sized constrained.\n");
    exit(2);
}

int main(int argc, char* argv[])
{
    /* Display usage if no arguments given */
    if(argc == 1)
        usage();

    /* Parse options */
    --argc, ++argv;
    while(argc > 0 && argv[0][0] == '-') {
        if(!strcmp(*argv, "--help"))
            usage();
        else if(!strcmp(*argv, "--nrv2b"))
            method = 0x2b;
        else if(!strcmp(*argv, "--nrv2d"))
            method = 0x2d;
        else if(!strcmp(*argv, "--nrv2e"))
            method = 0x2e;
        else if(!strcmp(*argv, "--level")) {
            --argc, ++argv;
            if(argc == 0) {
                fprintf(stderr, "option --level requires an argument\n");
                return 2;
            }

            char* endptr;
            level = strtol(*argv, &endptr, 0);
            if(*endptr) {
                fprintf(stderr, "bad argument to --level: '%s'\n", *argv);
                return 2;
            }
        } else {
            fprintf(stderr, "unknown option '%s'", *argv);
            return 2;
        }

        --argc, ++argv;
    }

    /* Parse positional arguments */
    if(argc == 0) {
        fprintf(stderr, "no input file specified\n");
        return 2;
    }

    char* infile_name = *argv;
    --argc, ++argv;

    if(argc == 0) {
        fprintf(stderr, "no output file specified\n");
        return 2;
    }

    char* outfile_name = *argv;
    --argc, ++argv;

    if(argc != 0) {
        fprintf(stderr, "excess arguments on command line\n");
        return 2;
    }

    /* Do the work */
    uint8_t* inbuf = NULL, *outbuf = NULL;
    FILE* outfile = NULL;
    size_t insize, outsize;
    uint8_t sizeb[4];
    int rc = 1;

    inbuf = load_file(infile_name, &insize);
    if(!inbuf)
        goto done;
    else if(insize > UINT32_MAX) {
        fprintf(stderr, "input file too big\n");
        goto done;
    }

    outbuf = uclpack(inbuf, insize, &outsize);
    if(!outbuf)
        goto done;
    else if(outsize > UINT32_MAX) {
        fprintf(stderr, "output too big\n");
        goto done;
    }

    outfile = fopen(outfile_name, "wb");
    if(!outfile) {
        fprintf(stderr, "cannot open output file: %s\n", outfile_name);
        goto done;
    }

    write_be32(sizeb, outsize);
    if(fwrite(sizeb, 4, 1, outfile) != 1)
        goto out_err;

    write_be32(sizeb, insize);
    if(fwrite(sizeb, 4, 1, outfile) != 1)
        goto out_err;

    if(fwrite(outbuf, outsize, 1, outfile) != 1) {
      out_err:
        fprintf(stderr, "error writing output file: %s\n", outfile_name);
        goto done;
    }

    rc = 0;

  done:
    if(inbuf)
        free(inbuf);
    if(outbuf)
        free(outbuf);
    if(outfile) {
        fclose(outfile);
        if(rc != 0)
            remove(outfile_name);
    }

    return rc;
}
