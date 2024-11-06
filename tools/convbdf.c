/*
 * Convert BDF files to C source and/or Rockbox .fnt file format
 *
 * Copyright (c) 2002 by Greg Haerr <greg@censoft.com>
 *
 * What fun it is converting font data...
 *
 * 09/17/02    Version 1.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define ROTATE /* define this for the new, rotated format */

/* BEGIN font.h */
/* loadable font magic and version number */
#ifdef ROTATE
#define VERSION     "RB12" /* newer version */
#else
#define VERSION     "RB11"
#endif

/*
 * bitmap_t helper macros
 */
typedef unsigned short bitmap_t; /* bitmap image unit size */

/* Number of words to hold a pixel line of width x pixels */
#define BITMAP_BITSPERIMAGE     (sizeof(bitmap_t) * 8)
#define BITMAP_WORDS(x)         (((x)+BITMAP_BITSPERIMAGE-1)/BITMAP_BITSPERIMAGE)
#define BITMAP_BYTES(x)         (BITMAP_WORDS(x)*sizeof(bitmap_t))
#define BITMAP_BITVALUE(n)      ((bitmap_t) (((bitmap_t) 1) << (n)))
#define BITMAP_FIRSTBIT         (BITMAP_BITVALUE(BITMAP_BITSPERIMAGE - 1))
#define BITMAP_TESTBIT(m)       ((m) & BITMAP_FIRSTBIT)
#define BITMAP_SHIFTBIT(m)      ((bitmap_t) ((m) << 1))


/* builtin C-based proportional/fixed font structure */
/* based on The Microwindows Project http://microwindows.org */
struct font {
    int     maxwidth;   /* max width in pixels */
    int     height;     /* height in pixels */
    int     ascent;     /* ascent (baseline) height */
    int     firstchar;  /* first character in bitmap */
    int     size;       /* font size in glyphs ('holes' included) */
    int     depth;      /* depth of the font, 0=1bit 1=4bit */
    bitmap_t*   bits;       /* 16-bit right-padded bitmap data */
    int*    offset;     /* offsets into bitmap data */
    unsigned char* width;   /* character widths or NULL if fixed */
    int     defaultchar;    /* default char (not glyph index) */
    int     bits_size;  /* # words of bitmap_t bits */
    
    /* unused by runtime system, read in by convbdf */
    int     nchars;     /* number of different glyphs */
    int     nchars_declared; /* number of glyphs as declared in the header */
    int     ascent_declared; /* ascent as declared in the header */
    int     descent_declared; /* descent as declared in the header */
    int     max_char_ascent;  /* max. char ascent (before adjusting) */
    int     max_char_descent; /* max. char descent (before adjusting) */
    unsigned int* offrot;   /* offsets into rotated bitmap data */
    char*   name;       /* font name */
    char*   facename;   /* facename of font */
    char*   copyright;  /* copyright info for loadable fonts */
    int     pixel_size;
    int     descent;
    int     fbbw, fbbh, fbbx, fbby;

    /* Max 'overflow' of a char's ascent (descent) over the font's one */
    int max_over_ascent, max_over_descent;
    
    /* The number of clipped ascents/descents/total */
    int num_clipped_ascent, num_clipped_descent, num_clipped;

    /* default width in pixels (can be overwritten at char level) */
    int default_width;
};
/* END font.h */

/* Description of how the ascent/descent is allowed to grow */
struct stretch {
    int value;    /* The delta value (in pixels or percents) */
    int percent;  /* Is the value in percents (true) or pixels (false)? */
    int force;    /* MUST the value be set (true) or is it just a max (false) */
};

#define isprefix(buf,str)   (!strncmp(buf, str, strlen(str)))
#define strequal(s1,s2)     (!strcmp(s1, s2))

#define MAX(a,b)            ((a) > (b) ? (a) : (b))
#define MIN(a,b)            ((a) < (b) ? (a) : (b))

#ifdef ROTATE
#define ROTATION_BUF_SIZE 2048
#endif

/* Depending on the verbosity level some warnings are printed or not */
int verbosity_level = 0;
int trace = 0;

/* Prints a warning of the specified verbosity level. It will only be
   really printed if the level is >= the level set in the settings */
void print_warning(int level, const char *fmt, ...);
void print_error(const char *fmt, ...);
void print_info(const char *fmt, ...);
void print_trace(const char *fmt, ...);
#define VL_CLIP_FONT  1  /* Verbosity level for clip related warnings at font level */
#define VL_CLIP_CHAR  2  /* Verbosity level for clip related warnings at char level */
#define VL_MISC  1  /* Verbosity level for other warnings */

int gen_c = 0;
int gen_h = 0;
int gen_fnt = 0;
int gen_map = 1;
int start_char = 0;
int limit_char = 65535;
int oflag = 0;
char outfile[256];

struct stretch stretch_ascent  = { 0, 0, 1 }; /* Don't allow ascent to grow by default */
struct stretch stretch_descent = { 0, 0, 1 }; /* Don't allow descent to grow by default */


void usage(void);
void getopts(int *pac, char ***pav);
int convbdf(char *path);

void free_font(struct font* pf);
struct font* bdf_read_font(char *path);
int bdf_read_header(FILE *fp, struct font* pf);
int bdf_read_bitmaps(FILE *fp, struct font* pf);

/*
   Counts the glyphs and determines the max dimensions of glyphs
   (fills the fields nchars, maxwidth, max_over_ascent, max_over_descent).
   Returns 0 on failure or not-0 on success.
*/
int bdf_analyze_font(FILE *fp, struct font* pf);
void bdf_correct_bbx(int *width, int *bbx); /* Corrects bbx and width if bbx<0 */

/* Corrects the ascent and returns the new value (value to use) */
int adjust_ascent(int ascent, int overflow, struct stretch *stretch);

char * bdf_getline(FILE *fp, char *buf, int len);
bitmap_t bdf_hexval(unsigned char *buf, int ndx1, int ndx2);

int gen_c_source(struct font* pf, char *path);
int gen_h_header(struct font* pf, char *path);
int gen_fnt_file(struct font* pf, char *path);

void
usage(void)
{
    /* We use string array because some C compilers issue warnings about too long strings */
    char *help[] = {
    "Usage: convbdf [options] [input-files]\n",
    "       convbdf [options] [-o output-file] [single-input-file]\n",
    "Options:\n",
    "    -c     Convert .bdf to .c source file\n",
    "    -h     Convert .bdf to .h header file (to create sysfont.h)\n",
    "    -f     Convert .bdf to .fnt font file\n",
    "    -s N   Start output at character encodings >= N\n",
    "    -l N   Limit output to character encodings <= N\n",
    "    -n     Don't generate bitmaps as comments in .c file\n",
    "    -a N[%][!] Allow the ascent to grow N pixels/% to avoid glyph clipping\n",
    "    -d N[%][!] Allow the descent to grow N pixels/% to avoid glyph clipping\n",
    "           An ! in the -a and -d options forces the growth; N may be negative\n",
    "    -v N   Verbosity level: 0=quite quiet, 1=more verbose, 2=even more, etc.\n",
    "    -t     Print internal tracing messages\n",
    NULL /* Must be the last element in the array */
    };

    char **p = help;
    while (*p != NULL)
        print_info("%s", *(p++));
}


void parse_ascent_opt(char *val, struct stretch *opt) {
    char buf[256];
    char *p;
    strcpy(buf, val);
    
    opt->force = 0;
    opt->percent = 0;
    p = buf + strlen(buf);
    while (p > buf) {
        p--;
        if (*p == '%') {
            opt->percent = 1;
            *p = '\0';
        }
        else if (*p == '!') {
            opt->force = 1;
            *p = '\0';
        }
        else {
            break;
        }
    }
    opt->value = atoi(buf);
}

/* parse command line options */
void getopts(int *pac, char ***pav)
{
    char *p;
    char **av;
    int ac;
    
    ac = *pac;
    av = *pav;
    while (ac > 0 && av[0][0] == '-') {
        p = &av[0][1]; 
        while( *p)
            switch(*p++) {
            case ' ':           /* multiple -args on av[] */
                while( *p && *p == ' ')
                    p++;
                if( *p++ != '-')    /* next option must have dash */
                    p = "";
                break;          /* proceed to next option */
            case 'c':           /* generate .c output */
                gen_c = 1;
                break;
            case 'h':           /* generate .h output */
                gen_h = 1;
                break;
            case 'f':           /* generate .fnt output */
                gen_fnt = 1;
                break;
            case 'n':           /* don't gen bitmap comments */
                gen_map = 0;
                break;
            case 'o':           /* set output file */
                oflag = 1;
                if (*p) {
                    strcpy(outfile, p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        strcpy(outfile, av[0]);
                }
                break;
            case 'l':           /* set encoding limit */
                if (*p) {
                    limit_char = atoi(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        limit_char = atoi(av[0]);
                }
                break;
            case 's':           /* set encoding start */
                if (*p) {
                    start_char = atoi(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        start_char = atoi(av[0]);
                }
                break;
            case 'a':           /* ascent growth */
                if (*p) {
                    parse_ascent_opt(p, &stretch_ascent);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        parse_ascent_opt(av[0], &stretch_ascent);
                }
                break;
            case 'd':           /* descent growth */
                if (*p) {
                    parse_ascent_opt(p, &stretch_descent);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        parse_ascent_opt(av[0], &stretch_descent);
                }
                break;
            case 'v':           /* verbosity */
                if (*p) {
                    verbosity_level = atoi(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        verbosity_level = atoi(av[0]);
                }
                break;
            case 't':           /* tracing */
                trace = 1;
                break;
            default:
                print_info("Unknown option ignored: %c\n", *(p-1));
            }
        ++av; --ac;
    }
    *pac = ac;
    *pav = av;
}

void print_warning(int level, const char *fmt, ...) {
    if (verbosity_level >= level) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, " WARN: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

void print_trace(const char *fmt, ...) {
    if (trace) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "TRACE: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

void print_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void print_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, " INFO: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/* remove directory prefix and file suffix from full path */
char *basename(char *path)
{
    char *p, *b;
    static char base[256];

    /* remove prepended path and extension */
    b = path;
    for (p=path; *p; ++p) {
        if (*p == '/')
            b = p + 1;
    }
    strcpy(base, b);
    for (p=base; *p; ++p) {
        if (*p == '.') {
            *p = 0;
            break;
        }
    }
    return base;
}

int convbdf(char *path)
{
    struct font* pf;
    int ret = 0;

    pf = bdf_read_font(path);
    if (!pf)
        exit(1);
    
    if (gen_c) {
        if (!oflag) {
            strcpy(outfile, basename(path));
            strcat(outfile, ".c");
        }
        ret |= gen_c_source(pf, outfile);
    }
    
    if (gen_h) {
        if (!oflag) {
            strcpy(outfile, basename(path));
            strcat(outfile, ".h");
        }
        ret |= gen_h_header(pf, outfile);
    }

    if (gen_fnt) {
        if (!oflag) {
            strcpy(outfile, basename(path));
            strcat(outfile, ".fnt");
        }
        ret |= gen_fnt_file(pf, outfile);
    }

    free_font(pf);
    return ret;
}

int main(int ac, char **av)
{
    int ret = 0;

    ++av; --ac;     /* skip av[0] */
    getopts(&ac, &av);  /* read command line options */

    if (ac < 1 || (!gen_c && !gen_h && !gen_fnt)) {
        usage();
        exit(1);
    }

    if (oflag) {
        if (ac > 1 || (gen_c && gen_fnt) || (gen_c && gen_h) || (gen_h && gen_fnt)) {
            usage();
            exit(1);
        }
    }
    
    while (ac > 0) {
        ret |= convbdf(av[0]);
        ++av; --ac;
    }
    
    exit(ret);
}

/* free font structure */
void free_font(struct font* pf)
{
    if (!pf)
        return;
    if (pf->name)
        free(pf->name);
    if (pf->facename)
        free(pf->facename);
    if (pf->bits)
        free(pf->bits);
    if (pf->offset)
        free(pf->offset);
    if (pf->offrot)
        free(pf->offrot);
    if (pf->width)
        free(pf->width);
    free(pf);
}

/* build incore structure from .bdf file */
struct font* bdf_read_font(char *path)
{
    FILE *fp;
    struct font* pf;

    fp = fopen(path, "rb");
    if (!fp) {
        print_error("Error opening file: %s\n", path);
        return NULL;
    }
    
    pf = (struct font*)calloc(1, sizeof(struct font));
    if (!pf)
        goto errout;
    memset(pf, 0, sizeof(struct font));
    
    pf->name = strdup(basename(path));

    if (!bdf_read_header(fp, pf)) {
        print_error("Error reading font header\n");
        goto errout;
    }
    print_trace("Read font header, nchars_decl=%d\n", pf->nchars_declared);

    if (!bdf_analyze_font(fp, pf)) {
        print_error("Error analyzing the font\n");
        goto errout;
    }
    print_trace("Analyzed font, nchars=%d, maxwidth=%d, asc_over=%d, desc_over=%d\n",
            pf->nchars, pf->maxwidth, pf->max_over_ascent, pf->max_over_descent);
    
    if (pf->nchars != pf->nchars_declared) {
        print_warning(VL_MISC, "The declared number of chars (%d) "
                "does not match the real number (%d)\n",
                pf->nchars_declared, pf->nchars);
    }
    
    /* Correct ascent/descent if necessary */
    pf->ascent = adjust_ascent(pf->ascent_declared, pf->max_over_ascent, &stretch_ascent);
    if (pf->ascent != pf->ascent_declared) {
        print_info("Font ascent has been changed from %d to %d\n",
                pf->ascent_declared, pf->ascent);
    }
    pf->descent = adjust_ascent(pf->descent, pf->max_over_descent, &stretch_descent);
    if (pf->descent != pf->descent_declared) {
        print_info("Font descent has been changed from %d to %d\n",
                pf->descent_declared, pf->descent);
    }
    pf->height = pf->ascent + pf->descent;
    if (pf->height != pf->ascent_declared + pf->descent_declared) {
        print_warning(VL_CLIP_FONT, "Generated font's height: %d\n", pf->height);
    }
    
    if (pf->ascent > pf->max_char_ascent) {
        print_trace("Font's ascent could be reduced by %d to %d without clipping\n",
                (pf->ascent - pf->max_char_ascent), pf->max_char_ascent);
    }
    if (pf->descent > pf->max_char_descent) {
        print_trace("Font's descent could be reduced by %d to %d without clipping\n",
                (pf->descent - pf->max_char_descent), pf->max_char_descent);
    }
    
    
    /* Alocate memory */
    pf->bits_size = pf->size * BITMAP_WORDS(pf->maxwidth) * pf->height;
    pf->bits = (bitmap_t *)malloc(pf->bits_size * sizeof(bitmap_t));
    pf->offset = (int *)malloc(pf->size * sizeof(int));
    pf->offrot = (unsigned int *)malloc(pf->size * sizeof(unsigned int));
    pf->width = (unsigned char *)malloc(pf->size * sizeof(unsigned char));
    
    if (!pf->bits || !pf->offset || !pf->offrot || !pf->width) {
        print_error("no memory for font load\n");
        goto errout;
    }

    pf->num_clipped_ascent = pf->num_clipped_descent = pf->num_clipped = 0;
    pf->max_over_ascent = pf->max_over_descent = 0;

    if (!bdf_read_bitmaps(fp, pf)) {
        print_error("Error reading font bitmaps\n");
        goto errout;
    }
    print_trace("Read bitmaps\n");
    
    if (pf->num_clipped > 0) {
        print_warning(VL_CLIP_FONT, "%d character(s) out of %d were clipped "
                "(%d at ascent, %d at descent)\n",
                pf->num_clipped, pf->nchars,
                pf->num_clipped_ascent, pf->num_clipped_descent);
        print_warning(VL_CLIP_FONT, "max overflows: %d pixel(s) at ascent, %d pixel(s) at descent\n",
                pf->max_over_ascent, pf->max_over_descent);
    }

    fclose(fp);
    return pf;

 errout:
    fclose(fp);
    free_font(pf);
    return NULL;
}

/* read bdf font header information, return 0 on error */
int bdf_read_header(FILE *fp, struct font* pf)
{
    int encoding;
    int firstchar = 65535;
    int lastchar = -1;
    char buf[256];
    char facename[256];
    char copyright[256];
    int is_header = 1;

    /* set certain values to errors for later error checking */
    pf->defaultchar = -1;
    pf->ascent = -1;
    pf->descent = -1;
    pf->default_width = -1;

    for (;;) {
        if (!bdf_getline(fp, buf, sizeof(buf))) {
            print_error("EOF on file\n");
            return 0;
        }
        if (isprefix(buf, "FONT ")) {       /* not required */
            if (sscanf(buf, "FONT %[^\n]", facename) != 1) {
                print_error("bad 'FONT'\n");
                return 0;
            }
            pf->facename = strdup(facename);
            continue;
        }
        if (isprefix(buf, "COPYRIGHT ")) {  /* not required */
            if (sscanf(buf, "COPYRIGHT \"%[^\"]", copyright) != 1) {
                print_error("bad 'COPYRIGHT'\n");
                return 0;
            }
            pf->copyright = strdup(copyright);
            continue;
        }
        if (isprefix(buf, "DEFAULT_CHAR ")) {   /* not required */
            if (sscanf(buf, "DEFAULT_CHAR %d", &pf->defaultchar) != 1) {
                print_error("bad 'DEFAULT_CHAR'\n");
                return 0;
            }
        }
        if (isprefix(buf, "FONT_DESCENT ")) {
            if (sscanf(buf, "FONT_DESCENT %d", &pf->descent_declared) != 1) {
                print_error("bad 'FONT_DESCENT'\n");
                return 0;
            }
            pf->descent = pf->descent_declared; /* For now */
            continue;
        }
        if (isprefix(buf, "FONT_ASCENT ")) {
            if (sscanf(buf, "FONT_ASCENT %d", &pf->ascent_declared) != 1) {
                print_error("bad 'FONT_ASCENT'\n");
                return 0;
            }
            pf->ascent = pf->ascent_declared; /* For now */
            continue;
        }
        if (isprefix(buf, "FONTBOUNDINGBOX ")) {
            if (sscanf(buf, "FONTBOUNDINGBOX %d %d %d %d",
                       &pf->fbbw, &pf->fbbh, &pf->fbbx, &pf->fbby) != 4) {
                print_error("bad 'FONTBOUNDINGBOX'\n");
                return 0;
            }
            continue;
        }
        if (isprefix(buf, "CHARS ")) {
            if (sscanf(buf, "CHARS %d", &pf->nchars_declared) != 1) {
                print_error("bad 'CHARS'\n");
                return 0;
            }
            continue;
        }
        if (isprefix(buf, "STARTCHAR")) {
            is_header = 0;
            continue;
        }

        /* for BDF version 2.2 */
        if (is_header && isprefix(buf, "DWIDTH ")) {
            if (sscanf(buf, "DWIDTH %d", &pf->default_width) != 1) {
                print_error("bad 'DWIDTH' at font level\n");
                return 0;
            }
            continue;
        }

        /*
         * Reading ENCODING is necessary to get firstchar/lastchar
         * which is needed to pre-calculate our offset and widths
         * array sizes.
         */
        if (isprefix(buf, "ENCODING ")) {
            if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
                print_error("bad 'ENCODING'\n");
                return 0;
            }
            if (encoding >= 0 &&
                encoding <= limit_char && 
                encoding >= start_char) {

                if (firstchar > encoding)
                    firstchar = encoding;
                if (lastchar < encoding)
                    lastchar = encoding;
            }
            continue;
        }
        if (strequal(buf, "ENDFONT"))
            break;
    }

    /* calc font height */
    if (pf->ascent < 0 || pf->descent < 0 || firstchar < 0) {
        print_error("Invalid BDF file, requires FONT_ASCENT/FONT_DESCENT/ENCODING\n");
        return 0;
    }
    pf->height = pf->ascent + pf->descent;

    /* calc default char */
    if (pf->defaultchar < 0 || 
        pf->defaultchar < firstchar || 
        pf->defaultchar > limit_char ||
        pf->defaultchar > lastchar)
        pf->defaultchar = firstchar;

    /* calc font size (offset/width entries) */
    pf->firstchar = firstchar;
    pf->size = lastchar - firstchar + 1;

    return 1;
}

/*
 * TODO: rework the code to avoid logics duplication in
 *       bdf_read_bitmaps and bdf_analyze_font
 */


/* read bdf font bitmaps, return 0 on error */
int bdf_read_bitmaps(FILE *fp, struct font* pf)
{
    int ofs = 0;
    int ofr = 0;
    int i, k, encoding, width;
    int bbw, bbh, bbx, bby;
    int proportional = 0;
    int encodetable = 0;
    int offset;
    char buf[256];
    bitmap_t *ch_bitmap;
    int ch_words;

    /* reset file pointer */
    fseek(fp, 0L, SEEK_SET);

    /* initially mark offsets as not used */
    for (i=0; i<pf->size; ++i)
        pf->offset[i] = -1;

    for (;;) {
        if (!bdf_getline(fp, buf, sizeof(buf))) {
            print_error("EOF on file\n");
            return 0;
        }
        if (isprefix(buf, "STARTCHAR")) {
            encoding = width = -1;
            bbw = pf->fbbw;
            bbh = pf->fbbh;
            bbx = pf->fbbx;
            bby = pf->fbby;
            continue;
        }
        if (isprefix(buf, "ENCODING ")) {
            if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
                print_error("bad 'ENCODING'\n");
                return 0;
            }
            if (encoding < start_char || encoding > limit_char)
                encoding = -1;
            continue;
        }
        if (isprefix(buf, "DWIDTH ")) {
            if (sscanf(buf, "DWIDTH %d", &width) != 1) {
                print_error("bad 'DWIDTH'\n");
                return 0;
            }
            /* use font boundingbox width if DWIDTH <= 0 */
            if (width <= 0)
                width = pf->fbbw - pf->fbbx;
            continue;
        }
        if (isprefix(buf, "BBX ")) {
            if (sscanf(buf, "BBX %d %d %d %d", &bbw, &bbh, &bbx, &bby) != 4) {
                print_error("bad 'BBX'\n");
                return 0;
            }
            continue;
        }
        if (strequal(buf, "BITMAP") || strequal(buf, "BITMAP ")) {
            int overflow_asc, overflow_desc;
            int bbh_orig, bby_orig, y;

            if (encoding < 0)
                continue;

            if (width < 0 && pf->default_width > 0)
                width = pf->default_width;

            /* set bits offset in encode map */
            if (pf->offset[encoding-pf->firstchar] != -1) {
                print_error("duplicate encoding for character %d (0x%02x), ignoring duplicate\n",
                        encoding, encoding);
                continue;
            }
            pf->offset[encoding-pf->firstchar] = ofs;
            pf->offrot[encoding-pf->firstchar] = ofr;

            /* calc char width */
            bdf_correct_bbx(&width, &bbx);
            pf->width[encoding-pf->firstchar] = width;

            ch_bitmap = pf->bits + ofs;
            ch_words = BITMAP_WORDS(width);
            memset(ch_bitmap, 0, BITMAP_BYTES(width) * pf->height); /* clear bitmap */

#define BM(row,col) (*(ch_bitmap + ((row)*ch_words) + (col)))
#define BITMAP_NIBBLES  (BITMAP_BITSPERIMAGE/4)

            bbh_orig = bbh;
            bby_orig = bby;

            overflow_asc = bby + bbh - pf->ascent;
            if (overflow_asc > 0) {
                pf->num_clipped_ascent++;
                if (overflow_asc > pf->max_over_ascent) {
                    pf->max_over_ascent = overflow_asc;
                }
                bbh = MAX(bbh - overflow_asc, 0); /* Clipped -> decrease the height */
                print_warning(VL_CLIP_CHAR, "character %d goes %d pixel(s)"
                        " beyond the font's ascent, it will be clipped\n",
                        encoding, overflow_asc);
            }
            overflow_desc = -bby - pf->descent;
            if (overflow_desc > 0) {
                pf->num_clipped_descent++;
                if (overflow_desc > pf->max_over_descent) {
                    pf->max_over_descent = overflow_desc;
                }
                bby += overflow_desc;
                bbh = MAX(bbh - overflow_desc, 0); /* Clipped -> decrease the height */
                print_warning(VL_CLIP_CHAR, "character %d goes %d pixel(s)"
                        " beyond the font's descent, it will be clipped\n",
                        encoding, overflow_desc);
            }
            if (overflow_asc > 0 || overflow_desc > 0) {
                pf->num_clipped++;
            }

            y = bby_orig + bbh_orig; /* 0-based y within the char */
            
            /* read bitmaps */
            for (i=0; ; ++i) {
                int hexnibbles;

                if (!bdf_getline(fp, buf, sizeof(buf))) {
                    print_error("EOF reading BITMAP data for character %d\n",
                            encoding);
                    return 0;
                }
                if (isprefix(buf, "ENDCHAR"))
                    break;
                
                y--;
                if ((y >= pf->ascent) || (y < -pf->descent)) {
                    /* We're beyond the area that Rockbox can render -> clip */
                    --i; /* This line doesn't count */
                    continue;
                }

                hexnibbles = strlen(buf);
                for (k=0; k<ch_words; ++k) {
                    int ndx = k * BITMAP_NIBBLES;
                    int padnibbles = hexnibbles - ndx;
                    bitmap_t value;
                    
                    if (padnibbles <= 0)
                        break;
                    if (padnibbles >= (int)BITMAP_NIBBLES)
                        padnibbles = 0;

                    value = bdf_hexval((unsigned char *)buf,
                                       ndx, ndx+BITMAP_NIBBLES-1-padnibbles);
                    value <<= padnibbles * BITMAP_NIBBLES;

                    BM(pf->height - pf->descent - bby - bbh + i, k) |=
                        value >> bbx;
                    /* handle overflow into next image word */
                    if (bbx) {
                        BM(pf->height - pf->descent - bby - bbh + i, k+1) =
                            value << (BITMAP_BITSPERIMAGE - bbx);
                    }
                }
            }

            ofs += BITMAP_WORDS(width) * pf->height;
            ofr += pf->width[encoding-pf->firstchar] * ((pf->height+7)/8);

            continue;
        }
        if (strequal(buf, "ENDFONT"))
            break;
    }

    /* change unused width values to default char values */
    for (i=0; i<pf->size; ++i) {
        int defchar = pf->defaultchar - pf->firstchar;

        if (pf->offset[i] == -1)
            pf->width[i] = pf->width[defchar];
    }

    /* determine whether font doesn't require encode table */
#ifdef ROTATE
    offset = 0;
    for (i=0; i<pf->size; ++i) {
        if ((int)pf->offrot[i] != offset) {
            encodetable = 1;
            break;
        }
        offset += pf->maxwidth * ((pf->height + 7) / 8);
    }
#else
    offset = 0;
    for (i=0; i<pf->size; ++i) {
        if (pf->offset[i] != offset) {
            encodetable = 1;
            break;
        }
        offset += BITMAP_WORDS(pf->width[i]) * pf->height;
    }
#endif
    if (!encodetable) {
        free(pf->offset);
        pf->offset = NULL;
    }

    /* determine whether font is fixed-width */
    for (i=0; i<pf->size; ++i) {
        if (pf->width[i] != pf->maxwidth) {
            proportional = 1;
            break;
        }
    }
    if (!proportional) {
        free(pf->width);
        pf->width = NULL;
    }

    /* reallocate bits array to actual bits used */
    if (ofs < pf->bits_size) {
        pf->bits = realloc(pf->bits, ofs * sizeof(bitmap_t));
        pf->bits_size = ofs;
    }

#ifdef ROTATE
    pf->bits_size = ofr; /* always update, rotated is smaller */
#endif

    return 1;
}

/* read the next non-comment line, returns buf or NULL if EOF */
char *bdf_getline(FILE *fp, char *buf, int len)
{
    int c;
    char *b;

    for (;;) {
        b = buf;
        while ((c = getc(fp)) != EOF) {
            if (c == '\r')
                continue;
            if (c == '\n')
                break;
            if (b - buf >= (len - 1))
                break;
            *b++ = c;
        }
        *b = '\0';
        if (c == EOF && b == buf)
            return NULL;
        if (b != buf && !isprefix(buf, "COMMENT"))
            break;
    }
    return buf;
}

void bdf_correct_bbx(int *width, int *bbx) {
    if (*bbx < 0) {
        /* Rockbox can't render overlapping glyphs */
        *width -= *bbx;
        *bbx = 0;
    }
}

int bdf_analyze_font(FILE *fp, struct font* pf) {
    char buf[256];
    int encoding;
    int width, bbw, bbh, bbx, bby, ascent, overflow;
    int read_enc = 0, read_width = 0, read_bbx = 0, read_endchar = 1;
    int ignore_char = 0;

    /* reset file pointer */
    fseek(fp, 0L, SEEK_SET);
    
    pf->maxwidth = 0;
    pf->nchars = 0;
    pf->max_char_ascent = pf->max_char_descent = 0;
    pf->max_over_ascent = pf->max_over_descent = 0;

    for (;;) {
        
        if (!bdf_getline(fp, buf, sizeof(buf))) {
            print_error("EOF on file\n");
            return 0;
        }
        if (isprefix(buf, "ENDFONT")) {
            if (!read_endchar) {
                print_error("No terminating ENDCHAR for character %d\n", encoding);
                return 0;
            }
            break;
        }
        if (isprefix(buf, "STARTCHAR")) {
            print_trace("Read STARTCHAR, nchars=%d, read_endchar=%d\n", pf->nchars, read_endchar);
            if (!read_endchar) {
                print_error("No terminating ENDCHAR for character %d\n", encoding);
                return 0;
            }
            read_enc = read_width = read_bbx = read_endchar = 0;
            continue;
        }
        if (isprefix(buf, "ENDCHAR")) {
            if (!read_enc) {
                print_error("ENCODING is not specified\n");
                return 0;
            }
            ignore_char = (encoding < start_char || encoding > limit_char);
            if (!ignore_char) {
                if (!read_width && pf->default_width > 0)
                {
                    width = pf->default_width;
                    read_width = 1;
                }
                if (!read_width || !read_bbx) {
                    print_error("WIDTH or BBX is not specified for character %d\n",
                            encoding);
                }
                bdf_correct_bbx(&width, &bbx);
                if (width > pf->maxwidth) {
                    pf->maxwidth = width;
                }

                ascent = bby + bbh;
                pf->max_char_ascent = MAX(pf->max_char_ascent, ascent);
                overflow = ascent - pf->ascent;
                pf->max_over_ascent = MAX(pf->max_over_ascent, overflow);

                ascent = -bby;
                pf->max_char_descent = MAX(pf->max_char_descent, ascent);
                overflow = ascent - pf->descent;
                pf->max_over_descent = MAX(pf->max_over_descent, overflow);
            }
            pf->nchars++;
            read_endchar = 1;
            continue;
        }
        if (isprefix(buf, "ENCODING ")) {
            if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
                print_error("bad 'ENCODING': '%s'\n", buf);
                return 0;
            }
            read_enc = 1;
            continue;
        }
        if (isprefix(buf, "DWIDTH ")) {
            if (sscanf(buf, "DWIDTH %d", &width) != 1) {
                print_error("bad 'DWIDTH': '%s'\n", buf);
                return 0;
            }
            /* use font boundingbox width if DWIDTH <= 0 */
            if (width < 0) {
                print_error("Negative char width: %d\n", width);
                return 0;
            }
            read_width = 1;
        }
        if (isprefix(buf, "BBX ")) {
            if (sscanf(buf, "BBX %d %d %d %d", &bbw, &bbh, &bbx, &bby) != 4) {
                print_error("bad 'BBX': '%s'\n", buf);
                return 0;
            }
            read_bbx = 1;
            continue;
        }
    }
    return 1;
}

int adjust_ascent(int ascent, int overflow, struct stretch *stretch) {
    int result;
    int px = stretch->value;
    if (stretch->percent) {
        px = ascent * px / 100;
    }
    
    if (stretch->force) {
        result = ascent + px;
    }
    else {
        result = ascent + MIN(overflow, px);
    }
    result = MAX(result, 0);
    return result;
}


/* return hex value of portion of buffer */
bitmap_t bdf_hexval(unsigned char *buf, int ndx1, int ndx2)
{
    bitmap_t val = 0;
    int i, c;

    for (i=ndx1; i<=ndx2; ++i) {
        c = buf[i];
        if (c >= '0' && c <= '9')
            c -= '0';
        else 
            if (c >= 'A' && c <= 'F')
                c = c - 'A' + 10;
            else 
                if (c >= 'a' && c <= 'f')
                    c = c - 'a' + 10;
                else 
                    c = 0;
        val = (val << 4) | c;
    }
    return val;
}


#ifdef ROTATE

/*
 * Take an bitmap_t bitmap and convert to Rockbox format.
 * Used for converting font glyphs for the time being.
 * Can use for standard X11 and Win32 images as well.
 * See format description in lcd-recorder.c
 *
 * Doing it this way keeps fonts in standard formats,
 * as well as keeping Rockbox hw bitmap format.
 *
 * Returns the size of the rotated glyph (in bytes) or a
 * negative value if the glyph could not be rotated.
 */
int rotleft(unsigned char *dst, /* output buffer */
            size_t dstlen,      /* buffer size */
            bitmap_t *src, unsigned int width, unsigned int height,
            int char_code)
{
    unsigned int i,j;
    unsigned int src_words;     /* # words of input image */
    unsigned int dst_mask;      /* bit mask for destination */
    bitmap_t src_mask;          /* bit mask for source */
    
    /* How large the buffer should be to hold the rotated bitmap
       of a glyph of size (width x height) */
    unsigned int needed_size = ((height + 7) / 8) * width;

    if (needed_size > dstlen) {
        print_error("Character %d: Glyph of size %d x %d can't be rotated "
                "(buffer size is %lu, needs %u)\n",
                char_code, width, height, (unsigned long)dstlen, needed_size);
        return -1;
    }

    /* calc words of input image */
    src_words = BITMAP_WORDS(width) * height;
    
    /* clear background */
    memset(dst, 0, needed_size);

    dst_mask = 1;

    for (i=0; i < src_words; i++) {

        /* calc src input bit */
        src_mask = 1 << (sizeof (bitmap_t) * 8 - 1);
        
        /* for each input column... */
        for(j=0; j < width; j++) {

            if (src_mask == 0) /* input word done? */
            {
                src_mask = 1 << (sizeof (bitmap_t) * 8 - 1);
                i++;    /* next input word */
            }

            /* if set in input, set in rotated output */
            if (src[i] & src_mask)
                dst[j] |= dst_mask;

            src_mask >>= 1;    /* next input bit */
        }

        dst_mask <<= 1;          /* next output bit (row) */
        if (dst_mask > (1 << 7)) /* output bit > 7? */
        {
            dst_mask = 1;
            dst += width;        /* next output byte row */
        }
    }
    return needed_size; /* return result size in bytes */
}

#endif /* ROTATE */


/* generate C source from in-core font */
int gen_c_source(struct font* pf, char *path)
{
    FILE *ofp;
    int i;
    time_t t = time(0);
#ifdef ROTATE
    int ofr = 0;
#else
    int did_syncmsg = 0;
    bitmap_t *ofs = pf->bits;
#endif
    char buf[256];
    char obuf[256];
    char hdr1[] = {
        "/* Generated by convbdf on %s. */\n"
        "#include <stdbool.h>\n"
        "#include \"config.h\"\n"
        "#include \"font.h\"\n"
        "\n"
        "/* Font information:\n"
        "   name: %s\n"
        "   facename: %s\n"
        "   w x h: %dx%d\n"
        "   size: %d\n"
        "   ascent: %d\n"
        "   descent: %d\n"
        "   depth: %d\n"
        "   first char: %d (0x%02x)\n"
        "   last char: %d (0x%02x)\n"
        "   default char: %d (0x%02x)\n"
        "   proportional: %s\n"
        "   %s\n"
        "*/\n"
        "\n"
        "/* Font character bitmap data. */\n"
        "static const unsigned char _font_bits[] = {\n"
    };

    ofp = fopen(path, "w");
    if (!ofp) {
        print_error("Can't create %s\n", path);
        return 1;
    }

    strcpy(buf, ctime(&t));
    buf[strlen(buf)-1] = 0;

    fprintf(ofp, hdr1, buf, 
            pf->name,
            pf->facename? pf->facename: "",
            pf->maxwidth, pf->height,
            pf->size,
            pf->ascent, pf->descent, pf->depth,
            pf->firstchar, pf->firstchar,
            pf->firstchar+pf->size-1, pf->firstchar+pf->size-1,
            pf->defaultchar, pf->defaultchar,
            pf->width? "yes": "no",
            pf->copyright? pf->copyright: "");

    /* generate bitmaps */
    for (i=0; i<pf->size; ++i) {
        int x;
        int bitcount = 0;
        int width = pf->width ? pf->width[i] : pf->maxwidth;
        int height = pf->height;
        int char_code = pf->firstchar + i;
        bitmap_t *bits;
        bitmap_t bitvalue=0;

        /* Skip missing glyphs */
        if (pf->offset && (pf->offset[i] == -1))
            continue;

        bits = pf->bits + (pf->offset? (int)pf->offset[i]: (pf->height * i));

        fprintf(ofp, "\n/* Character %d (0x%02x):\n   width %d",
                char_code, char_code, width);

        if (gen_map) {
            fprintf(ofp, "\n   +");
            for (x=0; x<width; ++x) fprintf(ofp, "-");
            fprintf(ofp, "+\n");

            x = 0;
            while (height > 0) {
                if (x == 0) fprintf(ofp, "   |");

                if (bitcount <= 0) {
                    bitcount = BITMAP_BITSPERIMAGE;
                    bitvalue = *bits++;
                }

                fprintf(ofp, BITMAP_TESTBIT(bitvalue)? "*": " ");

                bitvalue = BITMAP_SHIFTBIT(bitvalue);
                --bitcount;
                if (++x == width) {
                    fprintf(ofp, "|\n");
                    --height;
                    x = 0;
                    bitcount = 0;
                }
            }
            fprintf(ofp, "   +");
            for (x=0; x<width; ++x)
                fprintf(ofp, "-");
            fprintf(ofp, "+ */\n");
        }
        else
            fprintf(ofp, " */\n");

        bits = pf->bits + (pf->offset? (int)pf->offset[i]: (pf->height * i));
#ifdef ROTATE /* pre-rotated into Rockbox bitmap format */
        {
          unsigned char bytemap[ROTATION_BUF_SIZE];
          int y8, ix=0;
          
          int size = rotleft(bytemap, sizeof(bytemap), bits, width,
                             pf->height, char_code);
          if (size < 0) {
              return -1;
          }
          
          for (y8=0; y8<pf->height; y8+=8) /* column rows */
          {
            for (x=0; x<width; x++) {
                fprintf(ofp, "0x%02x, ", bytemap[ix]);
                ix++;
            }
            fprintf(ofp, "\n");
          }

          /* update offrot since bits are now in sorted order */
          pf->offrot[i] = ofr;
          ofr += size;
        }
#else
        for (x=BITMAP_WORDS(width)*pf->height; x>0; --x) {
            fprintf(ofp, "0x%04x,\n", *bits);
            if (!did_syncmsg && *bits++ != *ofs++) {
                print_warning(VL_MISC, "found encoding values in non-sorted order (not an error).\n");
                did_syncmsg = 1;
            }
        }
#endif
    }
    fprintf(ofp, "};\n\n");

    if (pf->offset) {
        /* output offset table */
        fprintf(ofp, "/* Character->glyph mapping. */\n"
                "static const unsigned %s _sysfont_offset[] = {\n", pf->bits_size > 0xffdb ? "long" : "short");

        for (i=0; i<pf->size; ++i) {
            int offset = pf->offset[i];
            int offrot = pf->offrot[i];
            if (offset == -1) {
                offset = pf->offset[pf->defaultchar - pf->firstchar];
                offrot = pf->offrot[pf->defaultchar - pf->firstchar];
            }
            fprintf(ofp, "  %d,\t/* (0x%02x) */\n", 
#ifdef ROTATE
                    offrot, i+pf->firstchar);
#else
                    offset, i+pf->firstchar);
#endif
        }
        fprintf(ofp, "};\n\n");
    }

    /* output width table for proportional fonts */
    if (pf->width) {
        fprintf(ofp, "/* Character width data. */\n"
                "static const unsigned char _sysfont_width[] = {\n");

        for (i=0; i<pf->size; ++i)
            fprintf(ofp, "  %d,\t/* (0x%02x) */\n", 
                    pf->width[i], i+pf->firstchar);
        fprintf(ofp, "};\n\n");
    }

    /* output struct font struct */
    if (pf->offset)
        sprintf(obuf, "_sysfont_offset,");
    else
        sprintf(obuf, "0,  /* no encode table */");

    if (pf->width)
        sprintf(buf, "_sysfont_width,  /* width */");
    else
        sprintf(buf, "0,  /* fixed width */");

    fprintf(ofp, "/* Exported structure definition. */\n"
            "const struct font sysfont = {\n"
            "  %d,  /* maxwidth */\n"
            "  %d,  /* height */\n"
            "  %d,  /* ascent */\n"
            "  %d,  /* firstchar */\n"
            "  %d,  /* size */\n"
            "  %d,  /* depth */\n"
            "  _font_bits, /* bits */\n"
            "  %s  /* offset */\n"
            "  %s\n"
            "  %d,  /* defaultchar */\n"
            "  %d,  /* bits_size */\n",
            pf->maxwidth, pf->height,
            pf->ascent,
            pf->firstchar,
            pf->size, 0,
            obuf,
            buf,
            pf->defaultchar,
            pf->bits_size
           );
    
    fprintf(ofp, "  -1,  /* font fd */\n"
            "  -1,  /* font fd width */\n"
            "  -1,  /* font fd offset */\n"
            "  -1,  /* font handle */\n"
            "  0,  /* buffer start */\n"
            "  0,  /* ^ position */\n"
            "  0,  /* ^ end */\n"
            "  0,  /* ^ size  */\n"
            " false, /* disabled */\n"
            "  {{0,0,0,0,0},0,0,0,0,0},   /* cache  */\n"
            "  0,  /*   */\n"
            "  0,  /*   */\n"
            "  0,  /*   */\n"
            "};\n"
          );

    return 0;
}

/* generate C header from in-core font */
int gen_h_header(struct font* pf, char *path)
{
    FILE *ofp;
    time_t t = time(0);
    char buf[256];
    char *hdr1 =
        "/* Generated by convbdf on %s. */\n"
        "\n"
        "/* Font information */\n"
        "#define SYSFONT_NAME           %s\n"
        "#define SYSFONT_FACENAME       %s\n"
        "#define SYSFONT_WIDTH          %d\n"
        "#define SYSFONT_HEIGHT         %d\n"
        "#define SYSFONT_SIZE           %d\n"
        "#define SYSFONT_ASCENT         %d\n"
        "#define SYSFONT_DEPTH          %d\n";
    char *hdr2 =
        "#define SYSFONT_DESCENT        %d\n"
        "#define SYSFONT_FIRST_CHAR     %d\n"
        "#define SYSFONT_LAST_CHAR      %d\n"
        "#define SYSFONT_DEFAULT_CHAR   %d\n"
        "#define SYSFONT_PROPORTIONAL   %d\n"
        "#define SYSFONT_COPYRIGHT      %s\n"
        "#define SYSFONT_BITS_SIZE      %d\n"
        "\n";

    ofp = fopen(path, "w");
    if (!ofp) {
        print_error("Can't create %s\n", path);
        return 1;
    }

    strcpy(buf, ctime(&t));
    buf[strlen(buf)-1] = 0;

    fprintf(ofp, hdr1, buf,
            pf->name,
            pf->facename? pf->facename: "",
            pf->maxwidth,
            pf->height,
            pf->size,
            pf->ascent,
            pf->depth);

    fprintf(ofp, hdr2, 
            pf->descent,
            pf->firstchar,
            pf->firstchar+pf->size-1,
            pf->defaultchar,
            pf->width? 1: 0,
            pf->copyright? pf->copyright: "",
            pf->bits_size);

    return 0;
}

static int writebyte(FILE *fp, unsigned char c)
{
    return putc(c, fp) != EOF;
}

static int writeshort(FILE *fp, unsigned short s)
{
    putc(s, fp);
    return putc(s>>8, fp) != EOF;
}

static int writeint(FILE *fp, unsigned int l)
{
    putc(l, fp);
    putc(l>>8, fp);
    putc(l>>16, fp);
    return putc(l>>24, fp) != EOF;
}

static int writestr(FILE *fp, char *str, int count)
{
    return (int)fwrite(str, 1, count, fp) == count;
}

#ifndef ROTATE
static int writestrpad(FILE *fp, char *str, int totlen)
{
    int ret = EOF;

    while (str && *str && totlen > 0) {
        if (*str) {
            ret = putc(*str++, fp);
            --totlen;
        }
    }
    while (--totlen >= 0)
        ret = putc(' ', fp);
    return ret;
}
#endif

/* generate .fnt format file from in-core font */
int gen_fnt_file(struct font* pf, char *path)
{
    FILE *ofp;
    int i;
#ifdef ROTATE
    int ofr = 0;
#endif

    ofp = fopen(path, "wb");
    if (!ofp) {
        print_error("Can't create %s\n", path);
        return 1;
    }

    /* write magic and version number */
    writestr(ofp, VERSION, 4);
#ifndef ROTATE
    /* internal font name */
    writestrpad(ofp, pf->name, 64);

    /* copyright */
    writestrpad(ofp, pf->copyright, 256);
#endif
    /* font info */
    writeshort(ofp, pf->maxwidth);
    writeshort(ofp, pf->height);
    writeshort(ofp, pf->ascent);
    writeshort(ofp, 0); /* depth = 0 for bdffonts */
    writeint(ofp, pf->firstchar);
    writeint(ofp, pf->defaultchar);
    writeint(ofp, pf->size);

    /* variable font data sizes */
    writeint(ofp, pf->bits_size);         /* # words of bitmap_t */
    writeint(ofp, pf->offset? pf->size: 0);  /* # ints of offset */
    writeint(ofp, pf->width? pf->size: 0);    /* # bytes of width */
    /* variable font data */
#ifdef ROTATE
    for (i=0; i<pf->size; ++i)
    {
        bitmap_t* bits;
        int width = pf->width ? pf->width[i] : pf->maxwidth;
        int size;
        int char_code = pf->firstchar + i;
        unsigned char bytemap[ROTATION_BUF_SIZE];

        /* Skip missing glyphs */
        if (pf->offset && (pf->offset[i] == -1))
            continue;

        bits = pf->bits + (pf->offset? (int)pf->offset[i]: (pf->height * i));

        size = rotleft(bytemap, sizeof(bytemap), bits, width, pf->height, char_code);
        if (size < 0) {
            return -1;
        }
        writestr(ofp, (char *)bytemap, size);

        /* update offrot since bits are now in sorted order */
        pf->offrot[i] = ofr;
        ofr += size;
    }

    if ( pf->bits_size < 0xFFDB )
    {
        /* bitmap offset is small enough, use unsigned short for offset */
        if (ftell(ofp) & 1)
            writebyte(ofp, 0);          /* pad to 16-bit boundary */
    }
    else
    {
        /* bitmap offset is large then 64K, use unsigned int for offset */
        while (ftell(ofp) & 3)
            writebyte(ofp, 0);          /* pad to 32-bit boundary */
    }

    if (pf->offset)
    {
        for (i=0; i<pf->size; ++i)
        {
            int offrot = pf->offrot[i];
            if (pf->offset[i] == -1) {
                offrot = pf->offrot[pf->defaultchar - pf->firstchar];
            }
            if ( pf->bits_size < 0xFFDB )
                writeshort(ofp, offrot);
            else
                writeint(ofp, offrot);
        }
    }

    if (pf->width)
        for (i=0; i<pf->size; ++i)
            writebyte(ofp, pf->width[i]);
#else
    for (i=0; i<pf->bits_size; ++i)
        writeshort(ofp, pf->bits[i]);
    if (ftell(ofp) & 2)
        writeshort(ofp, 0);     /* pad to 32-bit boundary */

    if (pf->offset)
        for (i=0; i<pf->size; ++i) {
            int offset = pf->offset[i];
            if (offset == -1) {
                offset = pf->offset[pf->defaultchar - pf->firstchar];
            }
            writeint(ofp, offset);
        }

    if (pf->width)
        for (i=0; i<pf->size; ++i)
            writebyte(ofp, pf->width[i]);
#endif
    fclose(ofp);
    return 0;
}
