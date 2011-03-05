/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonas Hurrelmann
 *
 * A command-line tool to convert ttf file to bitmap fonts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <stdbool.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#endif
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H

#include <string.h>
/*
 * Set the default values used to generate a BDF font.
 */
#ifndef DEFAULT_PLATFORM_ID
#define DEFAULT_PLATFORM_ID 3
#endif

#ifndef DEFAULT_ENCODING_ID
#define DEFAULT_ENCODING_ID 1
#endif

#define ABS(x)           (((x) < 0) ? -(x) : (x))

#define VERSION     "RB12"
/*
 * nameID macros for getting strings from the OT font.
 */
enum {
    BDFOTF_COPYRIGHT_STRING = 0,
    BDFOTF_FAMILY_STRING,
    BDFOTF_SUBFAMILY_STRING,
    BDFOTF_UNIQUEID_STRING,
    BDFOTF_FULLNAME_STRING,
    BDFOTF_VENDOR_STRING,
    BDFOTF_POSTSCRIPT_STRING,
    BDFOTF_TRADEMARK_STRING,
};
/*
 * String names for the string indexes. Used for error messages.
 */
static char *string_names[] = {
    "\"Copyright\"",
    "\"Family\"",
    "\"SubFamily\"",
    "\"Unique ID\"",
    "\"Full Name\"",
    "\"Vendor\"",
    "\"Postscript Name\"",
    "\"Trademark\""
};


/*
 * The default platform and encoding ID's.
 */
static int pid = DEFAULT_PLATFORM_ID;
static int eid = DEFAULT_ENCODING_ID;


/*
 * A flag indicating if a CMap was found or not.
 */
static FT_UShort nocmap;

int pct                     = 0; /* display ttc table if it is not zero. */
unsigned long   max_char    = 65535;
int             pixel_size  = 15;
unsigned long   start_char  = 0;
unsigned long   limit_char; 
unsigned long   firstchar   = 0;
unsigned long   lastchar;
FT_Long         ttc_index   = -1;
int             flg_all_ttc = 0;
short           antialias   = 1; /* smooth fonts with gray levels  */
int             oflag       = 0;
char            outfile[1024];
float           between_chr = 0.0f;
float           between_row = 0.0f;
int             hv_resolution = 60;
int             dump_glyphs = 0;
int             trimming    = 0;
int             trim_dp     = 0; /* trim descent percent */
int             trim_da     = 0; /* trim descnet actual  */
int             trim_ap     = 0; /* trim ascent  precent */
int             trim_aa     = 0; /* trim ascent  actual  */

struct font_header_struct {
    char header[4];                 /* magic number and version bytes */
    unsigned short maxwidth;        /* max width in pixels */
    unsigned short height;          /* height in pixels */
    unsigned short ascent;          /* ascent (baseline) height */
    unsigned short depth;           /* depth 0=1-bit, 1=4-bit */
    unsigned long firstchar;        /* first character in font */
    unsigned long defaultchar;      /* default character in font */
    unsigned long size;             /* # characters in font */
    unsigned long nbits;            /* # bytes imagebits data in file */ /* = bits_size */

    FT_Long noffset;          /* # longs offset data in file */
    FT_Long nwidth;           /* # bytes width data in file */
};

struct font_struct {
    struct font_header_struct header;
    unsigned char *chars_data;
    unsigned short *offset;
    FT_Long *offset_long;
    unsigned char *width;
};

struct ttc_table{
    FT_Long     ttc_count;
    char        **ttf_name;
};

/* exit the program with given message */
static void
panic( const char*  message)
{
    fprintf( stderr, "%s\n", message );
    exit( 1 );
}

static void
arg_panic( const char* message, const char* arg )
{
    fprintf( stderr, "%s: %s\n", message, arg );
    exit( 1 );
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

/* print usage information */
void usage(void)
{
    char help[] = {
        "Usage: convttf [options] [input-files]\n"
        "       convttf [options] [-o output-file] [single-input-file]\n\n"
        "       Default output-file : <font-size>-<basename>.fnt.\n"
        "                              When '-ta' or '-tc' is specified in command line,\n "
        "                               default output-file is: \n"
        "                             <font-size>-<internal postscript-name of input-file>.fnt.\n" 
        "Options:\n"
        "    -s N   Start output at character encodings >= N\n"
        "    -l N   Limit output to character encodings <= N\n"
        "    -p N   Font size N in pixel (default N=15)\n"
        "    -c N   Character separation in pixel.Insert space between lines.\n"
        "    -x     Trim glyphs horizontally of nearly empty space\n"
        "              (to improve spacing on V's W's, etc.)\n"
        "    -X     Set the horizontal and vertical resolution (default: 60)\n"
        "    -TA N  Trim vertical ascent  (N percent)\n"
        "    -TD N  Trim vertical descent (N percent)\n"
        "    -Ta N  Trim vertical ascent  (N pixels)\n"
        "    -Td N  Trim vertical descent (N pixels)\n"
        "    -r N   Row separation in pixel.Insert space between characters\n"
        "    -d     Debug: print converted glyph images\n"
        "    -tt    Display the True Type Collection tables available in the font\n"
        "    -t N   Index of true type collection. It must be start from 0.(default N=0).\n"
        "    -ta    Convert all fonts in ttc (ignores outfile option)\n"
    };
    fprintf(stderr, "%s", help);
    exit( 1 );
}

/* remove directory prefix and file suffix from full path*/
char *basename(char *path)
{
    char *p, *b;
    static char base[256];

    /* remove prepended path and extension*/
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


void setcharmap(FT_Face face)
{
    FT_Long i;

    /*
     * Get the requested cmap.
     */
    for (i = 0; i < face->num_charmaps; i++) {
        if (face->charmaps[i]->platform_id == pid &&
            face->charmaps[i]->encoding_id == eid)
          break;
    }

    if (i == face->num_charmaps && pid == 3 && eid == 1) {
        /*
         * Make a special case when this fails with pid == 3 and eid == 1.
         * Change to eid == 0 and try again.  This captures the two possible
         * cases for MS fonts.  Some other method should be used to cycle
         * through all the alternatives later.
         */
        for (i = 0; i < face->num_charmaps; i++) {
        if (face->charmaps[i]->platform_id == pid &&
            face->charmaps[i]->encoding_id == 0)
              break;
        }
        if (i < face->num_charmaps) {
            pid = 3;
            eid = 1;
            FT_Set_Charmap(face, face->charmaps[i]);
        } else {
            /*
             * No CMAP was found.
             */
            nocmap = 1;
            pid = eid = -1;
        }
    } else {
        FT_Set_Charmap(face, face->charmaps[i]);
        nocmap = 0;
    }

}

/*
 * quote in otf2bdf.
 * A generic routine to get a name from the OT name table.  This routine
 * always looks for English language names and checks three possibilities:
 * 1. English names with the MS Unicode encoding ID.
 * 2. English names with the MS unknown encoding ID.
 * 3. English names with the Apple Unicode encoding ID.
 *
 * The particular name ID mut be provided (e.g. nameID = 0 for copyright
 * string, nameID = 6 for Postscript name, nameID = 1 for typeface name.
 *
 * If the `dash_to_space' flag is non-zero, all dashes (-) in the name will be
 * replaced with the character passed.
 *
 * Returns the number of bytes added.
 */
static int
otf_get_english_string(FT_Face face, int nameID, int dash_to_space,
                       char *name, int name_size)
{

    int j, encid;
    FT_UInt i, nrec;
    FT_SfntName sfntName;
    unsigned char *s;
    unsigned short slen;

    nrec = FT_Get_Sfnt_Name_Count(face);

    for (encid = 1, j = 0; j < 2; j++, encid--) {
        /*
         * Locate one of the MS English font names.
         */
        for (i = 0; i < nrec; i++) {
           FT_Get_Sfnt_Name(face, i, &sfntName);
           if (sfntName.platform_id == 3 &&
               sfntName.encoding_id == encid &&
               sfntName.name_id == nameID &&
               (sfntName.language_id == 0x0409 ||
                sfntName.language_id == 0x0809 ||
                sfntName.language_id == 0x0c09 ||
                sfntName.language_id == 0x1009 ||
                sfntName.language_id == 0x1409 ||
                sfntName.language_id == 0x1809)) {
               s = sfntName.string;
               slen = sfntName.string_len;
               break;
           }
        }

        if (i < nrec) {
            if (slen >> 1 >= name_size) {
                fprintf(stderr, "warning: %s string longer than buffer."
                "Truncating to %d bytes.\n", string_names[nameID], name_size);
                slen = name_size << 1;
            }

            /*
             * Found one of the MS English font names.  The name is by
             * definition encoded in Unicode, so copy every second byte into
             * the `name' parameter, assuming there is enough space.
             */
            for (i = 1; i < slen; i += 2) {
                if (dash_to_space)
                  *name++ = (s[i] != '-') ? s[i] : ' ';
                else if (s[i] == '\r' || s[i] == '\n') {
                    if (s[i] == '\r' && i + 2 < slen && s[i + 2] == '\n')
                      i += 2;
                    *name++ = ' ';
                    *name++ = ' ';
                } else
                  *name++ = s[i];
            }
            *name = 0;
            return (slen >> 1);
        }
    }

    /*
     * No MS English name found, attempt to find an Apple Unicode English
     * name.
     */
    for (i = 0; i < nrec; i++) {
        FT_Get_Sfnt_Name(face, i, &sfntName);
        if (sfntName.platform_id == 0 && sfntName.language_id == 0 &&
            sfntName.name_id == nameID) {
            s = sfntName.string;
            slen = sfntName.string_len;
            break;
        }
    }

    if (i < nrec) {
        if (slen >> 1 >= name_size) {
            fprintf(stderr, "warning: %s string longer than buffer."
            "Truncating to %d bytes.\n", string_names[nameID], name_size);
            slen = name_size << 1;
        }

        /*
         * Found the Apple Unicode English name.  The name is by definition
         * encoded in Unicode, so copy every second byte into the `name'
         * parameter, assuming there is enough space.
         */
        for (i = 1; i < slen; i += 2) {
            if (dash_to_space)
              *name++ = (s[i] != '-') ? s[i] : ' ';
            else if (s[i] == '\r' || s[i] == '\n') {
                if (s[i] == '\r' && i + 2 < slen && s[i + 2] == '\n')
                  i += 2;
                *name++ = ' ';
                *name++ = ' ';
            } else
              *name++ = s[i];
        }
        *name = 0;
        return (slen >> 1);
    }

    return 0;
}


int get_ttc_table(char *path, struct ttc_table *ttcname )
{


    FT_Error    error;
    FT_Library  library;
    FT_Face     face;
    FT_Long     i;
    char        xlfd[BUFSIZ];

    /* init number of ttf in ttc */
    ttcname->ttc_count = 0;

    /* Initialize engine */
    if ( ( error = FT_Init_FreeType( &library ) ) != 0 ) 
    {
        panic( "Error while initializing engine" );
        return error;
    }


    /* Load face */
    error = FT_New_Face( library, path, (FT_Long) 0, &face );
    if ( error )
    {
        arg_panic( "Could not find/open font", path );
        return error;
    }

    ttcname->ttc_count = face->num_faces;
    ttcname->ttf_name = malloc( sizeof(char*) * ttcname->ttc_count);

    for(i = 0; i < ttcname->ttc_count; i++)
    {
        error = FT_New_Face( library, path, i, &face );
        if ( error == FT_Err_Cannot_Open_Stream )
            arg_panic( "Could not find/open font", path );
        otf_get_english_string(face, BDFOTF_POSTSCRIPT_STRING, 0, xlfd,
                                  sizeof(xlfd));
        ttcname->ttf_name[i] = malloc(sizeof(char) * (strlen(xlfd) + 1 ));
        strcpy(ttcname->ttf_name[i], xlfd);
    }
    return 0;
}

void print_ttc_table(char* path)
{
    struct ttc_table ttcname;
    FT_Long     i;

    get_ttc_table(path, &ttcname);
    printf("ttc header count = %ld \n\n", ttcname.ttc_count);
    printf("Encoding tables available in the true type collection\n\n");
    printf("INDEX\tPOSTSCRIPT NAME\n");
    printf("-----------------------------------------------------\n");
    for(i = 0; i < ttcname.ttc_count; i++)
    {
        printf("%ld\t%s\n", i, ttcname.ttf_name[i]);
    }
    for(i = 0; i < ttcname.ttc_count; i++)
    {
        free(ttcname.ttf_name[i]);
    }
    printf("\n\n");
    free(ttcname.ttf_name);

    return;
}

FT_Long getcharindex(FT_Face face, FT_Long code)
{
    FT_Long idx;
    if (nocmap) {
        if (code >= face->num_glyphs)
            idx = 0;
        else
            idx = code;
    } else
        idx = FT_Get_Char_Index( face, code);

    if ( idx <= 0 || idx > face->num_glyphs)
        return 0;
    else
        return idx;
}

void print_raw_glyph( FT_Face face)
{
    int pixel,row,col,width;
    width = face->glyph->metrics.width >> 6;

    printf("\n---Raw-Glyph---\n");
    for(row=0; row < face->glyph->metrics.height >> 6; row++) 
    {
        printf("_");
        for(col=0; col < width; col++)
        { 
            pixel = *(face->glyph->bitmap.buffer+width*row+col)/26;
            if ( pixel ) printf("%d",pixel); else printf(" ");
        }
        printf("_\n");
    }
    printf("----End-----\n");
}

int glyph_width( FT_Face face) 
{
    int pitch, h_adv, width;
    unsigned spacing = (unsigned)(between_chr * (1<<6));/* convert to fixed point */

    pitch = ABS(face->glyph->bitmap.pitch);
    h_adv = face->glyph->metrics.horiAdvance >> 6;
    width = (face->glyph->metrics.width + spacing) >> 6;

    if(pitch == 0)    pitch = h_adv;
    if(width < pitch) width = pitch;
    if(width == 0)    return 0;

    return width;
}


void trim_glyph( FT_GlyphSlot glyph, int *empty_first_col, 
                int *empty_last_col, int *width )
{
    int row;
    int stride = glyph->bitmap.pitch;
    int end = stride-1;
    int trim_left = 2, trim_right = 2;

    const unsigned char limit = 64u;
    const unsigned char *image = glyph->bitmap.buffer;

    if (*width < 2)
        return; /* nothing to do? */

    for(row=0; row< glyph->metrics.height >> 6; row++)
    {
        const unsigned char *column = image+row*stride;
        if (*column++ < limit && trim_left)
        {
            if (*column >= limit/2)
                trim_left = 1;
        }
        else
            trim_left = 0;

        column = image+row*stride+end;
        if (*column-- < limit && trim_right)
        {
            if (*column >= limit/2)
                trim_right = 1;
        }
        else
            trim_right = 0;
    }


    (*width) -= trim_left + trim_right;
    if (*width < 0) *width = 0;

    *empty_first_col = trim_left;
    *empty_last_col = trim_right;
}

void convttf(char* path, char* destfile, FT_Long face_index)
{
    FT_Error    error;
    FT_Library  library;
    FT_Face     face;
    int         w,h;
    int         row,col;
    int         empty_first_col, empty_last_col;
    FT_Long     charindex;
    FT_Long     index = 0;
    FT_Long     code;

    int depth = 2;
    unsigned char bit_shift = 1 << depth;
    unsigned char pixel_per_byte = CHAR_BIT / bit_shift;

    /* Initialize engine */
    if ( ( error = FT_Init_FreeType( &library ) ) != 0 )
        panic( "Error while initializing engine" );

    /* Load face */
    error = FT_New_Face( library, path, (FT_Long) face_index, &face );
    if ( error == FT_Err_Cannot_Open_Stream )
        arg_panic( "Could not find/open font\n", path );
    else if ( error )
        arg_panic( "Error while opening font\n", path );


    setcharmap( face );
    /* Set font header data */
    struct font_struct export_font;
    export_font.header.header[0] = 'R';
    export_font.header.header[1] = 'B';
    export_font.header.header[2] = '1';
    export_font.header.header[3] = '2';
    //export_font.header.height = 0;
    //export_font.header.ascent = 0;

    float extra_space = (float)(between_row-trim_aa-trim_da);
    FT_Set_Char_Size( face, 0, pixel_size << 6, hv_resolution, hv_resolution );
    export_font.header.ascent = 
            ((face->size->metrics.ascender*(100-trim_ap)/100) >> 6) - trim_aa;
            
    export_font.header.height = 
        (((face->size->metrics.ascender*(100-trim_ap)/100) -
        (face->size->metrics.descender*(100-trim_dp)/100)) >> 6) + extra_space;

    printf("\n");
    printf("Please wait, converting %s:\n", path);

    /* "face->num_glyphs" is NG.; */
    if ( limit_char == 0 ) limit_char = max_char;
    if ( limit_char > max_char ) limit_char = max_char;

    FT_Long char_count = 0;



    export_font.header.maxwidth = 1;
    export_font.header.depth = 1;
    firstchar = limit_char;
    lastchar  = start_char;

    /* calculate memory usage */
    for(code = start_char; code <= limit_char ; code++ )
    {
        charindex = getcharindex( face, code);
        if ( !(charindex) ) continue;
        error = FT_Load_Glyph( face, charindex, 
                               (FT_LOAD_RENDER | FT_LOAD_NO_BITMAP) );
        if ( error ) continue;
        
        w = glyph_width( face );
        if (w == 0) continue;
        empty_first_col = empty_last_col = 0;
        if(trimming)
            trim_glyph( face->glyph, &empty_first_col, &empty_last_col, &w);
        
        if (export_font.header.maxwidth < w) 
            export_font.header.maxwidth = w;


        char_count++;
        index += (w*export_font.header.height + pixel_per_byte - 1)/pixel_per_byte;

        if (code >= lastchar)
            lastchar = code;

        if (code <= firstchar)
            firstchar = code;
    }
    export_font.header.defaultchar = firstchar;
    export_font.header.firstchar = firstchar;
    export_font.header.size = lastchar - firstchar + 1;
    export_font.header.nbits = index;
    export_font.header.noffset = export_font.header.size;
    export_font.header.nwidth  = export_font.header.size;
    
    /* check if we need to use long offsets */
    char use_long_offset = (export_font.header.nbits >= 0xFFDB );

    /* allocate memory */
    export_font.offset = NULL;
    export_font.offset_long = NULL;
    if (use_long_offset)
        export_font.offset_long = 
            malloc( sizeof(FT_Long)* export_font.header.noffset );
    else
        export_font.offset = 
            malloc( sizeof(unsigned short)* export_font.header.noffset );

    export_font.width = 
            malloc( sizeof(unsigned char) * export_font.header.nwidth );
    export_font.chars_data = 
            malloc( sizeof(unsigned char) * export_font.header.nbits );
            
    /* for now we use the full height for each character */
    h = export_font.header.height;

    index = 0;
    int done = 0;
    char char_name[1024];
    int converted_char_count = 0;
    int failed_char_count = 0;

    for( code = firstchar; code <= lastchar; code++ )
    {
        /* Get gylph index from the char and render it */
        charindex = getcharindex( face, code);
        if ( !charindex ) 
        {
            if ( use_long_offset )
                export_font.offset_long[code - firstchar] = export_font.offset_long[0];
            else
                export_font.offset[code - firstchar] = export_font.offset[0];
            export_font.width[code - firstchar] = export_font.width[0];
            continue;
        }
        
        error = FT_Load_Glyph( face, charindex , 
                               (FT_LOAD_RENDER | FT_LOAD_NO_BITMAP) );
        if ( error ) {
            continue;
        }
        if FT_HAS_GLYPH_NAMES( face )
            FT_Get_Glyph_Name( face, charindex, char_name, 16);
        else
            char_name[0] = '\0';

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap* source = &slot->bitmap;
        //print_raw_glyph( face );
        w = glyph_width( face );
        if (w == 0) continue;
        empty_first_col = empty_last_col = 0;

        if(trimming)
            trim_glyph( face->glyph, &empty_first_col, &empty_last_col, &w );
        
        if ( use_long_offset )
            export_font.offset_long[code - firstchar] = index;
        else
            export_font.offset[code - firstchar] = index;

        export_font.width[code - firstchar] = w;

        /* copy the glyph bitmap to a full sized glyph bitmap */
        unsigned char* src = source->buffer;
        unsigned char* tmpbuf = malloc(sizeof(unsigned char) * w * h);
        memset(tmpbuf, 0xff, w*h);
        int start_y =  export_font.header.ascent - slot->bitmap_top;

        int glyph_height = source->rows;
        int stride = source->pitch;
        unsigned char* buf = tmpbuf;
        unsigned char* endbuf = tmpbuf + w*h;

        int error = 0;
        /* insert empty pixels on the left */
        int col_off = w - stride;
        if (col_off > 1) col_off /= 2;
        if (col_off < 0) col_off = 0;

        for(row=0; row < glyph_height; row++)
        {
            if(row+start_y < 0 || row+start_y >= h)
                continue;
            for(col = empty_first_col; col < stride; col++)
            {
                unsigned char *tsrc, *dst;
                dst = buf + (w*(start_y+row)) + col + col_off;
                tsrc = src + stride*row + col;
                if (dst < endbuf && dst >= tmpbuf)
                    *dst = 0xff - *tsrc;
                else {
                    error = 1;
                    printf("Error! row: %3d col: %3d\n", row, col);
                }
            }
        }
        if(error) print_raw_glyph(face);

        buf = tmpbuf;
        int numbits;
        unsigned int field;
        field = 0;
        numbits = pixel_per_byte;

        for(row=0; row < h; row++) 
        {
            for(col=0; col < w; col++)
            {
                unsigned int src = *buf++;
                unsigned int cur_col = (src + 8) / 17;
                field |= (cur_col << (bit_shift*(pixel_per_byte-numbits)));

                if (--numbits == 0)
                {
                    export_font.chars_data[index++] = (unsigned char)field;
                    numbits = pixel_per_byte;
                    field = 0;
                }
            }
        }

        /* Pad last byte */
        if (numbits != pixel_per_byte)
        {
            export_font.chars_data[index++] = (unsigned char)field;
        }

        if( dump_glyphs )
        {
            /* debug: dump char */
            printf("\n---Converted Glyph Dump---\n");
            unsigned char bit_max = (1 << bit_shift) - 1;
            if ( code > 32 && code < 255 ) {
                row = h;
                if(use_long_offset)
                    buf =  &(export_font.chars_data[export_font.offset_long[
                             code - firstchar]]);
                else
                    buf =  &(export_font.chars_data[export_font.offset[
                             code - firstchar]]);
                unsigned char current_data;
                unsigned char font_bits;
                numbits = pixel_per_byte;
                current_data = *buf;
                do
                {
                    col =  w;
                    printf("-");
                    do
                    {
                        font_bits = current_data & bit_max;
                        if (font_bits==bit_max)
                            printf(" ");
                        else
                        {
                            if(font_bits > bit_max/2)
                                printf(".");
                            else
                                printf("@");
                        }
                        if (--numbits == 0)
                        {
                            current_data = *(++buf);
                            numbits = pixel_per_byte;
                        }
                        else
                        {
                            current_data >>= bit_shift;
                        }
                    } while (--col);
                    printf("-\n");
                } while (--row);
            }
            buf = NULL;
            printf("---End Glyph Dump---\n");
        }

        free(tmpbuf);
        converted_char_count++;
        done = (100*(converted_char_count))/char_count;
        printf("Converted %s %d (%d%%)\e[K\r",
               char_name,converted_char_count,done); fflush(stdout);
    }

    FILE *file = fopen(destfile, "w");
    printf("Writing %s\n", destfile);

    /* font info */
    writestr(file, VERSION, 4);
    writeshort(file, export_font.header.maxwidth);
    writeshort(file, export_font.header.height);
    writeshort(file, export_font.header.ascent);
    writeshort(file, export_font.header.depth);
    writeint(file, export_font.header.firstchar);
    writeint(file, export_font.header.defaultchar);
    writeint(file, export_font.header.size);
    writeint(file, export_font.header.nbits);
    writeint(file, export_font.header.noffset);
    writeint(file, export_font.header.nwidth);

    fwrite( (char*)export_font.chars_data, 1, 
               export_font.header.nbits, file);
    free(export_font.chars_data);

    int skip,i;
    char pad[] = {0,0,0,0};
    if ( use_long_offset ) 
    {
        skip = ((export_font.header.nbits + 3) & ~3) - 
            export_font.header.nbits;
        fwrite(pad, 1, skip, file); /* pad */
        for(i = 0; i < export_font.header.noffset; i++)
            writeint(file, export_font.offset_long[i]);
    }
    else 
    {
        skip = ((export_font.header.nbits + 1) & ~1) - 
            export_font.header.nbits;
        fwrite(pad, 1, skip, file); /* pad */
        for(i = 0; i < export_font.header.noffset; i++)
            writeshort(file, export_font.offset[i]);
    }

    for(i = 0; i < export_font.header.nwidth; i++)
        writebyte(file, export_font.width[i]);
    free(export_font.width);

    if ( use_long_offset ) 
        free(export_font.offset_long);
    else 
        free(export_font.offset);

    fclose(file);
    FT_Done_Face( face );
    FT_Done_FreeType( library );
    printf("done (converted %d glyphs, %d errors).\e[K\n\n",
           converted_char_count, failed_char_count);

}

void convttc(char* path)
{
    struct ttc_table ttcname;
    FT_Long          i;

    get_ttc_table(path, &ttcname);

    if (ttcname.ttc_count == 0) 
    {
        printf("This file is a not true type font.\n");
        return;
    }

    /* default */
    if (!flg_all_ttc && ttc_index    == -1) 
    {
        if (!oflag)
        {   /* generate filename */
            snprintf(outfile, sizeof(outfile),
                     "%d-%s.fnt", pixel_size, basename(path));
        }
        convttf(path, outfile, (FT_Long) 0);
    }

    /* set face_index of ttc */
    else if (!flg_all_ttc)
    {
        print_ttc_table(path);
        if ( !oflag ) 
        {
            if (ttc_index >= 0 && 
                ttc_index < ttcname.ttc_count)
            {
                if (strcmp(ttcname.ttf_name[ttc_index], "") != 0) 
                {
                    snprintf(outfile, sizeof(outfile), "%d-%s.fnt",
                             pixel_size, ttcname.ttf_name[ttc_index]);
                }
                else
                {
                    snprintf(outfile, sizeof(outfile), "%d-%s-%ld.fnt",
                             pixel_size, basename(path), ttc_index);
                }
            }
            else
            {
                printf("illegal face index of ttc.\n");
            }
        }
        convttf(path, outfile, ttc_index);
    }
    else { /* convert all fonts */
        print_ttc_table(path);
        for(i = 0; i < ttcname.ttc_count; i++)
        {
            snprintf(outfile, sizeof(outfile), "%d-%s.fnt",
                     pixel_size, ttcname.ttf_name[i]);
            convttf(path, outfile, i);
        }
    }

    for(i = 0; i < ttcname.ttc_count; i++)
    {
        free(ttcname.ttf_name[i]);
    }
    free(ttcname.ttf_name);
}



/* parse command line options*/
void getopts(int *pac, char ***pav)
{
    char *p;
    char **av;
    int ac;
    ac = *pac;
    av = *pav;

    limit_char  = max_char;
    start_char  = 0;

    while (ac > 0 && av[0][0] == '-') {
        p = &av[0][1]; 
        while( *p)
            switch(*p++) {
            case 'h':case 'H':
                usage();
                break;
            case ' ':           /* multiple -args on av[]*/
                while( *p && *p == ' ')
                    p++;
                if( *p++ != '-')    /* next option must have dash*/
                    p = "";
                break;          /* proceed to next option*/
            case 'o':       /* set output file*/
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
            case 'l':       /* set encoding limit*/
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
            case 's':       /* set encoding start*/
                if (*p) {
                    start_char = atol(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        start_char = atol(av[0]);
                }
                break;
            case 'p':       /* set pixel size*/
                if (*p) {
                    pixel_size = atoi(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        pixel_size = atoi(av[0]);
                }
                break;
            case 'c':       /* set spaece between characters */
                {
                    if (*p) {
                        between_chr = atof(p);
                        while (*p && *p != ' ')
                            p++;
                    }
                    else {
                        av++; ac--;
                        if (ac > 0)
                            between_chr = atof(av[0]);
                    }
                    break;
                }
            case 'd':
                dump_glyphs = 1;
                while (*p && *p != ' ')
                    p++;
                break;
            case 'x':
                trimming = 1;
                while (*p && *p != ' ')
                    p++;
                break;
            case 'X':
                if (*p) {
                    hv_resolution = atoi(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        hv_resolution = atoi(av[0]);
                }
                break;
            case 'r':
                if (*p) {
                    between_row = atof(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        between_row = atof(av[0]);
                }
                break;
            case 'T':
                  if(*p == 'A') {
                      if(*(++p)) {
                          trim_ap = atoi(p);
                          while (*p && *p != ' ')
                              p++;
                      }
                      else {
                          av++; ac--;
                          if (ac > 0)
                              trim_ap = atoi(av[0]);
                      }
                      break;
                  }
                  if(*p == 'D') {
                      if(*(++p)) {
                          trim_dp = atoi(p);
                          while (*p && *p != ' ')
                              p++;
                      }
                      else {
                          av++; ac--;
                          if (ac > 0)
                              trim_dp = atoi(av[0]);
                      }
                      break;
                  }
                  if(*p == 'a') {
                      if(*(++p)) {
                          trim_aa = atoi(p);
                          while (*p && *p != ' ')
                              p++;
                      }
                      else {
                          av++; ac--;
                          if (ac > 0)
                              trim_aa = atoi(av[0]);
                      }
                      break;
                  }
                  if(*p == 'd') {
                      if(*(++p)) {
                          trim_da = atoi(p);

                      }
                      else {
                          av++; ac--;
                          if (ac > 0)
                              trim_da = atoi(av[0]);
                      }
                      break;
                  }
                  fprintf(stderr, "Unknown option ignored: %s\n", p-1);
                  while (*p && *p != ' ')
                      p++;
                  break;
            case 't':     /* display ttc table */
                if (*p == 't') {
                    pct = 1;
                    while (*p && *p != ' ')
                        p++;
                }

                else if (*p == 'a') {
                    flg_all_ttc = 1;
                    while (*p && *p != ' ')
                        p++;
                }

                else if (*p) {
                    ttc_index = atoi(p);
                    while (*p && *p != ' ')
                        p++;
                }
                else {
                    av++; ac--;
                    if (ac > 0)
                        ttc_index = atoi(av[0]);
                }
                break;

            default:
                fprintf(stderr, "Unknown option ignored: %s\n", p-1);
                while (*p && *p != ' ')
                    p++;
            }
        ++av; --ac;
    }
    *pac = ac;
    *pav = av;
}


int main(int ac, char **av)
{
    int ret = 0;

    ++av; --ac;     /* skip av[0]*/

    getopts(&ac, &av);  /* read command line options*/

    if (ac < 1) 
    {
        usage();
    }
    if (oflag) 
    {
        if (ac > 1) 
        {
            usage();
        }
    }

    if (limit_char < start_char)
    {
        usage();
        exit(0);
    }

    while (pct && ac > 0) 
    {
        print_ttc_table(av[0]);
        ++av; --ac;
        exit(0);
    }

    while (ac > 0) 
    {
        convttc(av[0]);
        ++av; --ac;
    }

    exit(ret);
}



/*
 * Trie node structure.
 */
typedef struct {
    unsigned short key; /* Key value.                   */
    unsigned short val; /* Data for the key.                */
    unsigned long sibs; /* Offset of siblings from trie beginning.  */
    unsigned long kids; /* Offset of children from trie beginning.  */
} node_t;

/*
 * The trie used for remapping codes.
 */
static node_t *nodes;
static unsigned long nodes_used = 0;

int 
otf2bdf_remap(unsigned short *code)
{
    unsigned long i, n, t;
    unsigned short c, codes[2];

    /*
     * If no mapping table was loaded, then simply return the code.
     */
    if (nodes_used == 0)
      return 1;

    c = *code;
    codes[0] = (c >> 8) & 0xff;
    codes[1] = c & 0xff;

    for (i = n = 0; i < 2; i++) {
        t = nodes[n].kids;
        if (t == 0)
          return 0;
        for (; nodes[t].sibs && nodes[t].key != codes[i]; t = nodes[t].sibs);
        if (nodes[t].key != codes[i])
          return 0;
        n = t;
    }

    *code = nodes[n].val;
    return 1;
}
