/*
 * bdf2bmp  --  output all glyphs in a bdf-font to a bmp-image-file
 * version: 0.6
 * date:    Wed Jan 10 23:59:03 2001
 * author:  ITOU Hiroki (itouh@lycos.ne.jp)
 */

/*
 * Copyright (c) 2000,2001 ITOU Hiroki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* modify if you want; color of spacing */
#define COLOR_SPACING_RED 0x9a
#define COLOR_SPACING_GREEN 0x9a
#define COLOR_SPACING_BLUE 0xbd
/* modify if you want; out-of-dwidth over baseline */
#define COLOR_OVERBL_RED 0xca
#define COLOR_OVERBL_GREEN 0xca
#define COLOR_OVERBL_BLUE 0xd8
/* modify if you want; out-of-dwidth under baseline */
#define COLOR_UNDERBL_RED 0xde
#define COLOR_UNDERBL_GREEN 0xde
#define COLOR_UNDERBL_BLUE 0xe7

#define VERBOSE

#include <stdio.h>  /* printf(), fopen(), fwrite() */
#include <stdlib.h> /* malloc(), EXIT_SUCCESS, strtol(), exit() */
#include <string.h> /* strcmp(), strcpy() */
#include <limits.h> /* strtol() */
#include <sys/stat.h> /* stat() */
#include <sys/types.h> /* stat ? */
#include <ctype.h> /* isdigit() */

#define LINE_CHARMAX 1000 /* number of max characters in bdf-font-file; number is without reason */
#define FILENAME_CHARMAX 256 /* number of max characters in filenames;  number is without reason */
#define ON 1 /* number is without reason; only needs the difference to OFF */
#define OFF 0 /* number is without reason; only needs the difference to ON */
#define PARAM_MAX 10 /* max number of parameters */

#ifdef DEBUG
#define d_printf(message,arg) printf(message,arg)
#else /* not DEBUG */
#define d_printf(message,arg)
#endif /* DEBUG */

#ifdef VERBOSE
#define v_printf(message,arg) printf(message,arg)
#else /* not VERBOSE */
#define v_printf(message,arg)
#endif /* VERBOSE */

/* macro */
#define STOREBITMAP()   if(flagBitmap == ON){\
                            memcpy(nextP, sP, length);\
                            nextP += length;\
                        }

struct boundingbox{
        int w; /* width (pixel) */
        int h; /* height */
        int offx; /* offset y (pixel) */
        int offy; /* offset y */
};

/* global var */
struct boundingbox font; /* global boundingbox */
static int chars; /* total number of glyphs in a bdf file */
static int dwflag = ON; /* device width; only used for proportional-fonts */
static int endian; /* 0 = MSB, 1 = LSB */

/* func prototype */
void checkEndian(void);
void dwrite(const void *ptrP, int n, FILE *outP);
void writeBmpFile(unsigned char *bitmapP, int spacing, int colchar, FILE *bmpP);
void assignBitmap(unsigned char *bitmapP, char *glyphP, int sizeglyphP, struct boundingbox glyph, int dw);
int getline(char* lineP, int max, FILE* inputP);
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP);
void printhelp(void);
int main(int argc, char *argv[]);



/*
 * Is your-CPU-byte-order MSB or LSB?
 * MSB .. Most Significant Byte first (BigEndian)  e.g. PowerPC, SPARC
 * LSB .. Least Significant Byte first (LittleEndian) e.g. Intel Pentium
 */
void checkEndian(void){
        unsigned long ulong = 0x12345678;
        unsigned char *ucharP;

        ucharP = (unsigned char *)(&ulong);
        if(*ucharP == 0x78){
                d_printf("LSB 0x%x\n", *ucharP);
                endian = 1;
        }else{
                d_printf("MSB 0x%x\n", *ucharP);
                endian = 0;
        }
}



/*
 * write to disk; with arranging LSBfirst(LittleEndian) byte order,
 *                 because BMP-file is defined as LSB-first
 */
void dwrite(const void *varP, int n, FILE *outP){
        const unsigned char *p = varP;
        int i;
        unsigned char tmp;

        if(endian == 1){
                /* LSB; write without arranging */
                for(i=0; i<n; i++){
                        tmp = fwrite(p+i, 1, sizeof(unsigned char), outP);
                        if(tmp != sizeof(unsigned char)){
                                printf("error: cannot write an output-file\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }else{
                /* MSB; write with arranging */
                for(i=n-1; i>=0; i--){
                        tmp = fwrite(p+i, 1, sizeof(unsigned char), outP);
                        if(tmp != sizeof(unsigned char)){
                                printf("error: cannot write an output-file\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }
}



/*
 * 3. read bitmapAREA(onMemory) and write bmpFile with adding spacing
 *    BMP-file: noCompression(BI_RGB), 8bitColor, Windows-Win32 type
 */
void writeBmpFile(unsigned char *bitmapP, int spacing, int colchar, FILE *bmpP){
        long bmpw; /* bmp-image width (pixel) */
        long bmph; /* bmp-image height (pixel) */
        int bmppad; /* number of padding pixels */
        unsigned long bmpTotalSize; /* bmp filesize (byte) */
        /*  bmp-lines needs to be long alined and padded with 0 */
        unsigned long ulong;
        unsigned short ushort;
        signed long slong;
        unsigned char uchar;
        int i,x,y,g,tmp;
        int rowchar; /* number of row glyphs */
        int bx, by;

        /* bmp-image width */
        bmpw = (font.w+spacing)*colchar + spacing;

        /* bmp-image height */
        rowchar = (chars/colchar) + (chars%colchar!=0);
        bmph = (font.h+spacing)*rowchar + spacing;

        v_printf("  BMP width = %d pixels\n", (int)bmpw);
        v_printf("  BMP height = %d pixels\n", (int)bmph);
        d_printf("  number of glyphs column=%d ", colchar);
        d_printf("row=%d\n", rowchar);

        bmppad = ((bmpw + 3) / 4 * 4) - bmpw;
        bmpTotalSize = (bmpw + bmppad) * bmph
                + sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
        v_printf("  BMP filesize = %d bytes\n", (int)bmpTotalSize);


        /*
         * BITMAPFILEHEADER struct
         */
        ushort = 0x4d42; /* 4d = 'M', 42 = 'B' */
        dwrite(&ushort, sizeof(ushort), bmpP);
        ulong = bmpTotalSize;
        dwrite(&ulong, sizeof(ulong), bmpP);
        ushort = 0x00;
        dwrite(&ushort, sizeof(ushort), bmpP); /* reserved as 0 */
        dwrite(&ushort, sizeof(ushort), bmpP); /* reserved as 0 */

        /* bfOffBits: offset to image-data array */
        ulong = sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
        dwrite(&ulong, sizeof(ulong), bmpP);


        /*
         * BITMAPINFOHEADER struct
         */
        ulong = 40; /* when Windows-BMP, this is 40 */
        dwrite(&ulong, sizeof(ulong), bmpP);
        slong = bmpw; /* biWidth */
        dwrite(&slong, sizeof(slong), bmpP);
        slong = bmph; /* biHeight: down-top */
        dwrite(&slong, sizeof(slong), bmpP);
        ushort = 1; /* biPlanes: must be 1 */
        dwrite(&ushort, sizeof(ushort), bmpP);
        ushort = 8; /* biBitCount: 8bitColor */
        dwrite(&ushort, sizeof(ushort), bmpP);
        ulong = 0; /* biCompression: noCompression(BI_RGB) */
        dwrite(&ulong, sizeof(ulong), bmpP);
        ulong = 0; /* biSizeImage: when noComprsn, 0 is ok */
        dwrite(&ulong, sizeof(ulong), bmpP);
        slong = 0; /* biXPelsPerMeter: resolution x; 0 ok */
        dwrite(&slong, sizeof(slong), bmpP);
        slong = 0; /* biYPelsPerMeter: res y; 0 is ok */
        dwrite(&slong, sizeof(slong), bmpP);
        ulong = 0; /* biClrUsed: optimized color palette; not used */
        dwrite(&ulong, sizeof(ulong), bmpP);
        ulong = 0; /* biClrImportant: 0 is ok */
        dwrite(&ulong, sizeof(ulong), bmpP);

        /*
         * RGBQUAD[256]: color palette
         */
        /*   palette[0]: background of glyphs */
        uchar = 0xff;
        dwrite(&uchar, sizeof(uchar), bmpP); /* rgbBlue: B */
        dwrite(&uchar, sizeof(uchar), bmpP); /* rgbGreen: G */
        dwrite(&uchar, sizeof(uchar), bmpP); /* rgbRed: R */
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); /* rgbReserved: must be 0 */

        /*   palette[1]: foreground of glyphs */
        uchar = 0;
        for(i=0; i<4; i++)
                dwrite(&uchar, sizeof(uchar), bmpP); /* palette[1]: #000000 */

        /*   palette[2]: spacing */
        uchar = COLOR_SPACING_BLUE;
        dwrite(&uchar, sizeof(uchar), bmpP); /* B */
        uchar = COLOR_SPACING_GREEN;
        dwrite(&uchar, sizeof(uchar), bmpP); /* G */
        uchar = COLOR_SPACING_RED;
        dwrite(&uchar, sizeof(uchar), bmpP); /* R */
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); /* must be 0 */

        /*   palette[3]: out of dwidth over baseline */
        uchar = COLOR_OVERBL_BLUE;
        dwrite(&uchar, sizeof(uchar), bmpP); /* B */
        uchar = COLOR_OVERBL_GREEN;
        dwrite(&uchar, sizeof(uchar), bmpP); /* G */
        uchar = COLOR_OVERBL_RED;
        dwrite(&uchar, sizeof(uchar), bmpP); /* R */
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); /* must be 0 */

        /*   palette[4]: out of dwidth; under baseline */
        uchar = COLOR_UNDERBL_BLUE;
        dwrite(&uchar, sizeof(uchar), bmpP); /* B */
        uchar = COLOR_UNDERBL_GREEN;
        dwrite(&uchar, sizeof(uchar), bmpP); /* G */
        uchar = COLOR_UNDERBL_RED;
        dwrite(&uchar, sizeof(uchar), bmpP); /* R */
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); /* must be 0 */

        /*   palette[5] to palette[255]: not used */
        for(i=5; i<256; i++){
                uchar = 0x00; /* palette[5to255]: #000000 */
                dwrite(&uchar, sizeof(uchar), bmpP);
                dwrite(&uchar, sizeof(uchar), bmpP);
                dwrite(&uchar, sizeof(uchar), bmpP);
                dwrite(&uchar, sizeof(uchar), bmpP);
        }

        /*
         * IMAGE DATA array
         */
        for(y=bmph-1; y>=0; y--){
                for(x=0; x<bmpw+bmppad; x++){
                        if(x>=bmpw){
                                /* padding: long(4bytes) aligned */
                                uchar = 0; /* must pad with 0 */
                                dwrite(&uchar, sizeof(uchar), bmpP);
                        }else{
                                if( (y%(font.h+spacing)<spacing) || (x%(font.w+spacing)<spacing) ){
                                        /* spacing */
                                        uchar = 2; /* fill palette[2] */
                                        dwrite(&uchar, sizeof(uchar), bmpP);
                                }else{
                                        /* read bitmapAREA & write bmpFile */
                                        g = (x/(font.w+spacing)) + (y/(font.h+spacing)*colchar);
                                        bx = x - (spacing*(g%colchar)) - spacing;
                                        by = y - (spacing*(g/colchar)) - spacing;
                                        tmp = g*(font.h*font.w) + (by%font.h)*font.w + (bx%font.w);
                                        if(tmp >= chars*font.h*font.w){
                                                /* spacing over the last glyph */
                                                uchar = 2; /* fill palette[2] */
                                        }else
                                                uchar = *( bitmapP + tmp);
                                        dwrite(&uchar, sizeof(uchar), bmpP);
                                }
                        }
                }
        }
        return;
}




/*
 * 2. transfer bdf-font-file to bitmapAREA(onMemory) one glyph by one
 */
void assignBitmap(unsigned char *bitmapP, char *glyphP, int sizeglyphP, struct boundingbox glyph, int dw){
        static char *hex2binP[]= {
                "0000","0001","0010","0011","0100","0101","0110","0111",
                "1000","1001","1010","1011","1100","1101","1110","1111"
        };
        int d; /* decimal number translated from hexNumber */
        int hexlen; /* a line length(without newline code) */
        char binP[LINE_CHARMAX]; /* binary strings translated from decimal number */
        static int nowchar = 0; /* number of glyphs handlled until now */
        char *tmpP;
        char tmpsP[LINE_CHARMAX];
        int bitAh, bitAw; /* bitA width, height */
        int offtop, offbottom, offleft; /* glyph offset */
        unsigned char *bitAP;
        unsigned char *bitBP;
        int i,j,x,y;

        /*
         * 2.1) change hexadecimal strings to a bitmap of glyph( called bitA)
         */
        tmpP = strstr(glyphP, "\n");
        if(tmpP == NULL){
                /* if there is BITMAP\nENDCHAR in a given bdf-file */
                *glyphP = '0';
                *(glyphP+1) = '0';
                *(glyphP+2) = '\n';
                tmpP = glyphP + 2;
                sizeglyphP = 3;
        }
        hexlen = tmpP - glyphP;
        bitAw = hexlen * 4;
        bitAh = sizeglyphP / (hexlen+1);
        bitAP = malloc(bitAw * bitAh); /* address of bitA */
        if(bitAP == NULL){
                printf("error bitA malloc\n");
                exit(EXIT_FAILURE);
        }
        for(i=0,x=0,y=0; i<sizeglyphP; i++){
                if(glyphP[i] == '\n'){
                        x=0; y++;
                }else{
                        sprintf(tmpsP, "0x%c", glyphP[i]); /* get one character from hexadecimal strings */
                        d = (int)strtol(tmpsP,(char **)NULL, 16);
                        strcpy(binP, hex2binP[d]); /* change hexa strings to bin strings */
                        for(j=0; j<4; j++,x++){
                                if( bitAP+y*bitAw+x > bitAP+bitAw*bitAh ){
                                        printf("error: bitA pointer\n");
                                        exit(EXIT_FAILURE);
                                }else{
                                        *(bitAP + y*bitAw + x) = binP[j] - '0';
                                }
                        }
                }
        }

        /*
         * 2.2)make another bitmap area(called bitB)
         *      bitB is sized to FONTBOUNDINGBOX
         */
        bitBP = malloc(font.w * font.h); /* address of bitB */
        if(bitBP == NULL){
                printf("error bitB malloc\n");
                exit(EXIT_FAILURE);
        }
        for(i=0; i<font.h; i++){
                for(j=0; j<font.w; j++){
                        if(dwflag == OFF){
                                /* all in boundingbox: palette[0] */
                                *(bitBP + i*font.w + j) = 0;
                        }else{
                                /* show the baselines and widths of glyphs */
                                if( (j < (-1)*font.offx) || (j >= (-1)*font.offx+dw)){
                                        if(i < font.h -  (-1)*font.offy){
                                                /* over baseline: palette[3] */
                                                *(bitBP + i*font.w + j) = 3;
                                        }else{
                                                /* under baseline: palette[4] */
                                                *(bitBP + i*font.w + j) = 4;
                                        }
                                }else{
                                        /* in dwidth: palette[0] */
                                        *(bitBP + i*font.w + j) = 0;
                                }
                        }
                }
        }

        /*
         * 2.3) copy bitA inside BBX (smaller) to bitB (bigger)
         *       with offset-shifting;
         *      a scope beyond bitA is already filled with palette[0or3]
         */
        offleft = (-1)*font.offx + glyph.offx;
        /* offright = font.w - glyph.w - offleft; */

        offbottom = (-1)*font.offy + glyph.offy;
        offtop = font.h - glyph.h - offbottom;

        for(i=0; i<font.h; i++){
                if( i<offtop || i>=offtop+glyph.h )
                        ; /* do nothing */
                else
                        for(j=0; j<font.w; j++)
                                if( j<offleft || j>=offleft+glyph.w )
                                        ; /* do nothing */
                                else
                                        *(bitBP + i*font.w + j) = *(bitAP + (i-offtop)*bitAw + (j-offleft));
        }

        /*
         * 2.4) copy bitB to bitmapAREA
         */
        for(i=0; i<font.h; i++)
                for(j=0; j<font.w; j++)
                        *(bitmapP + (nowchar*font.w*font.h) + (i*font.w) + j) = *(bitBP + i*font.w + j);

        nowchar++;
        free(bitAP);
        free(bitBP);
}


/*
 * read oneline from textfile
 */
int getline(char* lineP, int max, FILE* inputP){
        if (fgets(lineP, max, inputP) == NULL)
                return 0;
        else
                return strlen(lineP); /* fgets returns strings included '\n' */
}


/*
 * 1. read BDF-file and transfer to assignBitmap()
 */
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP){
        int i;
        int length;
        char sP[LINE_CHARMAX]; /* one line(strings) from bdf-font-file */
        static int cnt; /* only used in debugging: counter of appeared glyphs */
        struct boundingbox glyph; /* an indivisual glyph width, height,offset x,y */
        int flagBitmap = OFF; /* this line is bitmap-data?(ON) or no?(OFF) */
        char *tokP; /* top address of a token from strings */
        char *glyphP = NULL; /* bitmap-data(hexadecimal strings) */
        char* nextP = NULL; /* address of writing next in glyphP */
        int dw = 0; /* dwidth */
        static int bdfflag = OFF; /* the given bdf-file is valid or not */

        while(1){
                length = getline(sP, LINE_CHARMAX, readP);
                if((bdfflag == OFF) && (length == 0)){
                        /* given input-file is not a bdf-file */
                        printf("error: input-file is not a bdf file\n");
                        exit(EXIT_FAILURE);
                }
                if(length == 0)
                        break; /* escape from while-loop */

                /* remove carriage-return(CR) */
                for(i=0; i<length; i++){
                        if(sP[i] == '\r'){
                                sP[i] = '\n';
                                length--;
                        }
                }

                /* classify from the top character of sP */
                switch(sP[0]){
                case 'S':
                        if(bdfflag == OFF){
                                /* top of the bdf-file */
                                if(strncmp(sP, "STARTFONT ", 10) == 0){
                                        bdfflag = ON;
                                        d_printf("startfont exists %d\n", bdfflag);
                                }else{
                                        /* given input-file is not a bdf-file */
                                        printf("error: input-file is not a bdf file\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        break;
                case 'F':
                        if(strncmp(sP, "FONTBOUNDINGBOX ", 16) == 0){
                                /*  16 means no comparing '\0' */

                                /* get font.w, font.h, font.offx, and font.offy */
                                tokP = strtok(sP, " ");/* tokP addresses next space of FONTBOUNDINGBOX */
                                tokP += (strlen(tokP)+1);/* tokP addresses top character of width in FONTBOUNDINGBOX */
                                tokP = strtok(tokP, " ");/* set NUL on space after width */
                                font.w = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* height in FONTBOUNDINGBOX */
                                tokP = strtok(tokP, " ");
                                font.h = atoi(tokP);
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, " ");
                                font.offx = atoi(tokP);
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, "\n");
                                font.offy = atoi(tokP);
                                d_printf("global glyph width=%dpixels ",font.w);
                                d_printf("height=%dpixels\n",font.h);
                                d_printf("global glyph offset x=%dpixels ",font.offx);
                                d_printf("y=%dpixels\n",font.offy);
                        }else
                                STOREBITMAP();
                        break;
                case 'C':
                        if(strncmp(sP, "CHARS ", 6) == 0){
                                /* get chars */
                                tokP = strtok(sP, " ");
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, "\n");
                                chars = atoi(tokP);
                                v_printf("  Total glyphs = %d\n",chars);
                                cnt=0;

                                /* allocate bitmapAREA */
                                bitmapP = (unsigned char*)malloc(chars * font.h * font.w );
                                if(bitmapP == NULL){
                                        printf("error malloc\n");
                                        exit(EXIT_FAILURE);
                                }
                        }else
                                STOREBITMAP();
                        break;
                case 'D':
                        if(strncmp(sP, "DWIDTH ", 7) == 0){
                                /* get dw */
                                tokP = strtok(sP, " ");
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, " ");
                                dw = atoi(tokP);
                        }else
                                STOREBITMAP();
                        break;
                case 'B':
                        if(strncmp(sP, "BITMAP", 6) == 0){
                                /* allocate glyphP */
                                glyphP = (char*)malloc(font.w*font.h); /* allocate more room */
                                if(glyphP == NULL){
                                        printf("error malloc bdf\n");
                                        exit(EXIT_FAILURE);
                                }
                                memset(glyphP, 0, font.w*font.h); /* zero clear */
                                nextP = glyphP;
                                flagBitmap = ON;
                        }else if(strncmp(sP, "BBX ", 4) == 0){
                                /* get glyph.offx, glyph.offy, glyph.w, and glyph.h */
                                tokP = strtok(sP, " ");/* space after 'BBX' */
                                tokP += (strlen(tokP)+1);/* top of width */
                                tokP = strtok(tokP, " ");/* set NUL on space after width */
                                glyph.w = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* height */
                                tokP = strtok(tokP, " ");
                                glyph.h = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* offx */
                                tokP = strtok(tokP, " ");
                                glyph.offx = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* offy */
                                tokP = strtok(tokP, "\n");
                                glyph.offy = atoi(tokP);
                                /* d_printf("glyph width=%dpixels ",glyph.w); */
                                /* d_printf("height=%dpixels\n",glyph.h); */
                                /* d_printf("glyph offset x=%dpixels ",glyph.offx); */
                                /* d_printf("y=%dpixels\n",glyph.offy); */
                        }else
                                STOREBITMAP();
                        break;
                case 'E':
                        if(strncmp(sP, "ENDCHAR", 7) == 0){
                                d_printf("\nglyph %d\n", cnt);
                                d_printf("%s\n",glyphP);
                                assignBitmap(bitmapP, glyphP, nextP - glyphP, glyph, dw);
                                flagBitmap = OFF;
                                free(glyphP);
                                cnt++;
                        }else
                                STOREBITMAP();
                        break;
                default:
                        STOREBITMAP();
                        break;
                }/* switch */
        }/* while */
        /* 'break' goto here */
        return bitmapP;
}


/*
 *
 */
void printhelp(void){
        printf("bdf2bmp version 0.6\n");
        printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
        printf("Option:\n");
        printf("  -sN    spacing N pixels (default N=2)\n");
        printf("             N value can range from 0 to 32\n");
        printf("  -cN    specifying N colomns in grid (default N=32)\n");
        printf("             N value can range from 1 to 1024\n");
        printf("  -w     showing the baseline and the widths of glyphs\n");
        printf("             with gray colors\n");
        printf("  -i     prompting whether to overwrite an existing file\n");
        printf("  -h     print help\n");
        exit(EXIT_FAILURE);
}



/*
 *
 */
int main(int argc, char *argv[]){
        FILE *readP;
        FILE *writeP;
        char readFilename[FILENAME_CHARMAX] = "input.bdf";
        char writeFilename[FILENAME_CHARMAX] = "output.bmp";
        int i, j, tmp, n, dst, c;
        char *sP;
        unsigned char *bitmapP = NULL; /* address of bitmapAREA */
        int spacing = 2; /* breadth of spacing (default 2) */
        int flag;
        int colchar = 32; /* number of columns(horizontal) (default 32) */
        char paramP[PARAM_MAX][LINE_CHARMAX]; /* parameter strings */
        int iflag = OFF;
        struct stat fileinfo;

        /*
         * deal with arguments
         */
        if(argc < 2){
                /* printf("error: not enough arguments\n"); */
                printhelp();
        }

        /* formatting arguments */
        sP = calloc(LINE_CHARMAX, sizeof(char));
        if(sP == NULL){
                printf("error\n");
                exit(1);
        }
        for(i=1,dst=0,n=0; i<argc; i++){
                if(argv[i][0] == '-'){
                        /* command-line options */
                        for(j=1; j<(int)strlen(argv[i]); j++){
                                if(argv[i][j]=='w' ||
                                   argv[i][j]=='i' ||
                                   argv[i][j]=='h' ||
                                   argv[i][j]=='v')
                                {
                                        *(sP+dst) = '-'; dst++;
                                        *(sP+dst) = argv[i][j]; dst++;
                                        *(sP+dst) = '\0';
                                        strcpy(paramP[n], sP); dst=0; n++;
                                        memset(sP,0,LINE_CHARMAX);
                                        if(n >= PARAM_MAX){
                                                printf("error: too many arguments\n");
                                                exit(EXIT_FAILURE);
                                        }

                                }else if( (argv[i][j]=='s') ||
                                          (argv[i][j]=='c'))
                                {
                                        *(sP+dst) = '-'; dst++;
                                        *(sP+dst) = argv[i][j]; dst++;
                                }else if( isdigit(argv[i][j]) == 0 ){
                                        /* not [0-9] */
                                        printf("error: invalid option -- '%c'\n", argv[i][j]);
                                        exit(EXIT_FAILURE);
                                }else if( argv[i][j+1] == '\0' ){
                                        *(sP+dst) = argv[i][j]; dst++;
                                        *(sP+dst) = '\0';
                                        strcpy(paramP[n], sP); dst=0; n++;
                                        if(n >= PARAM_MAX){
                                                printf("error: too many arguments\n");
                                                exit(EXIT_FAILURE);
                                        }
                                }else{
                                        *(sP+dst) = argv[i][j]; dst++;
                                }
                        }
                }else{
                        /* not command-line options */
                        for(j=0; j<(int)strlen(argv[i]); j++){
                                *(sP+dst) = argv[i][j]; dst++;
                        }
                        *(sP+dst) = '\0';
                        strcpy(paramP[n], sP); dst=0; n++;
                        memset(sP,0,LINE_CHARMAX);
                        if(n >= PARAM_MAX){
                                printf("error: too many arguments\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }
        free(sP);

        /* interpretting arguments */
        for(i=0, flag=0; i<n; i++){
                switch( paramP[i][0] ){
                case '-':
                        if(paramP[i][1] == 's')
                                spacing = atoi(&paramP[i][2]);
                        else if(paramP[i][1] == 'c')
                                colchar = atoi(&paramP[i][2]);
                        else if(paramP[i][1] == 'w')
                                dwflag = ON;
                        else if(paramP[i][1] == 'i')
                                iflag = ON;
                        else if( (paramP[i][1]=='v') || (paramP[i][1]=='h'))
                                printhelp();
                        break;
                default:
                        if(flag == 0){
                                strcpy(readFilename, paramP[i]);
                                flag ++;
                        }else{
                                strcpy(writeFilename, paramP[i]);
                                if(strcmp(readFilename, writeFilename) == 0){
                                        printf("error: input-filename and output-filename are same\n");
                                        exit(EXIT_FAILURE);
                                }
                                flag ++;
                        }
                        break;
                }
        }

        if(flag < 2){
                printf("error: not enough arguments\n");
                printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
                exit(EXIT_FAILURE);
        }

        /* colchar is limited from 1 to 1024 */
        if(colchar  < 1)
                colchar = 1;
        else if(colchar > 1024)
                colchar = 1024;

        /* spacing is limited from 0 to 32 */
        if(spacing < 0)
                spacing = 0;
        else if(spacing > 32)
                spacing = 32;

        /* checkEndian */
        checkEndian();

        /*
         * prepare to read&write files
         */
        readP = fopen(readFilename, "r");
        if(readP == NULL){
                printf("bdf2bmp: '%s' does not exist\n", readFilename);
                exit(EXIT_FAILURE);
        }
        /* Does writeFilename already exist? */
        if((iflag==ON) && (stat(writeFilename, &fileinfo)==0)){
                fprintf(stderr, "bdf2bmp: overwrite '%s'? ", writeFilename);
                c = fgetc(stdin);
                if((c=='y') || (c=='Y'))
                        ; /* go next */
                else
                        /* printf("not overwrite\n"); */
                        exit(EXIT_FAILURE);
        }
        writeP=fopen(writeFilename, "wb");
        if(writeP == NULL){
                printf("error: cannot write '%s'\n", writeFilename);
                exit(EXIT_FAILURE);
        }


        /* read bdf-font-file */
        bitmapP = readBdfFile(bitmapP, readP);
        fclose(readP);

        /* write bmp-image-file */
        writeBmpFile(bitmapP, spacing, colchar, writeP);
        tmp = fclose(writeP);
        if(tmp == EOF){
                printf("error: cannot write '%s'\n", writeFilename);
                free(bitmapP);
                exit(EXIT_FAILURE);
        }

        free(bitmapP);
        return EXIT_SUCCESS;
}
