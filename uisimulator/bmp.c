/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*********************************************************************
 *
 * Converts BMP files to Rockbox bitmap format
 *
 * 1999-05-03 Linus Nielsen Feltzing
 *
 **********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "file.h"

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#endif

struct Fileheader
{
  unsigned short Type;          /* signature - 'BM' */
  unsigned  long Size;          /* file size in bytes */
  unsigned short Reserved1;     /* 0 */
  unsigned short Reserved2;     /* 0 */
  unsigned long  OffBits;       /* offset to bitmap */
  unsigned long  StructSize;    /* size of this struct (40) */
  unsigned long  Width;         /* bmap width in pixels */
  unsigned long  Height;        /* bmap height in pixels */
  unsigned short Planes;        /* num planes - always 1 */
  unsigned short BitCount;      /* bits per pixel */
  unsigned long  Compression;   /* compression flag */
  unsigned long  SizeImage;     /* image size in bytes */
  long           XPelsPerMeter; /* horz resolution */
  long           YPelsPerMeter; /* vert resolution */
  unsigned long  ClrUsed;       /* 0 -> color table size */
  unsigned long  ClrImportant;  /* important color count */
} STRUCT_PACKED;

struct RGBQUAD
{
  unsigned char rgbBlue;
  unsigned char rgbGreen;
  unsigned char rgbRed;
  unsigned char rgbReserved;
} STRUCT_PACKED;

static struct Fileheader fh;
static unsigned char* bmp;
static struct RGBQUAD palette[2]; /* two colors only */

static unsigned int bitmap_width, bitmap_height;
static unsigned char *bitmap;

#ifdef STANDALONE
static id_str[256];
static bool compress = false;
static bool assembly = false;
static unsigned char* converted_bmp;
static unsigned char* compressed_bmp;
static unsigned int width;
static unsigned int converted_size;
static unsigned int compressed_size;
static unsigned int rounded_width;
#endif

#define readshort(x) (((x&0xff00)>>8)|((x&0x00ff)<<8))
#define readlong(x) (((x&0xff000000)>>24)| \
                     ((x&0x00ff0000)>>8) | \
                     ((x&0x0000ff00)<<8) | \
                     ((x&0x000000ff)<<24))
                     

/*********************************************************************
 * read_bmp_file()
 *
 * Reads a monochrome BMP file and puts the data in a 1-pixel-per-byte
 * array. Returns 0 on success.
 *
 **********************************************/
int read_bmp_file(char* filename,
                  int *get_width,  /* in pixels */
                  int *get_height, /* in pixels */
                  char *bitmap)
{
   long PaddedWidth;
   int background;
   int fd = open(filename, O_RDONLY);
   long size;
   unsigned int row, col, byte, bit;
   int l;
   char *bmp;
   int width;
   int height;

   if(fd == -1)
   {
      debugf("error - can't open '%s'\n", filename);
      return 1;
   }
   else
   {
     if(read(fd, &fh, sizeof(struct Fileheader)) !=
        sizeof(struct Fileheader))
      {
	debugf("error - can't Read Fileheader Stucture\n");
        return 2;
      }

      /* Exit if not monochrome */
      if(readshort(fh.BitCount) > 8)
      {
	debugf("error - Bitmap must be less than 8, got %d\n",
               readshort(fh.BitCount));
        return 2;
      }

      /* Exit if too wide */
      if(readlong(fh.Width) > 112)
      {
	debugf("error - Bitmap is too wide (%d pixels, max is 112)\n",
               readlong(fh.Width));
        return 3;
      }
      debugf("Bitmap is %d pixels wide\n", readlong(fh.Width));

      /* Exit if too high */
      if(readlong(fh.Height) > 64)
      {
	debugf("error - Bitmap is too high (%d pixels, max is 64)\n",
               readlong(fh.Height));
	return 4;
      }
      debugf("Bitmap is %d pixels heigh\n", readlong(fh.Height));

      for(l=0;l < 2;l++)
      {
	if(read(fd, &palette[l],sizeof(struct RGBQUAD)) !=
           sizeof(struct RGBQUAD))
	{
          debugf("error - Can't read bitmap's color palette\n");
          return 5;
	}
      }
      /* pass the other palettes */
      lseek(fd, 254*sizeof(struct RGBQUAD), SEEK_CUR);

      /* Try to guess the foreground and background colors.
         We assume that the foreground color is the darkest. */
      if(((int)palette[0].rgbRed +
          (int)palette[0].rgbGreen +
          (int)palette[0].rgbBlue) >
          ((int)palette[1].rgbRed +
          (int)palette[1].rgbGreen +
          (int)palette[1].rgbBlue))
      {
         background = 0;
      }
      else
      {
         background = 1;
      }

      width = readlong(fh.Width)*readshort(fh.BitCount);
      PaddedWidth = ((width+31)&(~0x1f))/8;
      size = PaddedWidth*readlong(fh.Height);

      bmp = (unsigned char *)malloc(size);

      if(bmp == NULL)
      {
          debugf("error - Out of memory\n");
          return 6;
      }
      else
      {
          if(read(fd, (unsigned char*)bmp,(long)size) != size) {
              debugf("error - Can't read image\n");
              return 7;
          }
      }

      bitmap_height = readlong(fh.Height);
      bitmap_width = readlong(fh.Width);

      *get_width = bitmap_width;
      *get_height = bitmap_height;

#if 0
      /* Now convert the bitmap into an array with 1 byte per pixel,
	exactly the size of the image */
      for(row = 0;row < bitmap_height;row++) {
	bit = 7;
	byte = 0;
	for(col = 0;col < bitmap_width;col++) {
          if((bmp[(bitmap_height - row - 1) * PaddedWidth + byte] &
              (1 << bit))) {

            bitmap[ (row/8) * bitmap_width + col ] |= 1<<(row&7);
          }
          else {
            bitmap[ (row/8) * bitmap_width + col ] &= ~ 1<<(row&7);
          }
          if(bit) {
            bit--;
          }
          else {
            bit = 7;
            byte++;
          }
	}
      }
#else
      /* Now convert the bitmap into an array with 1 byte per pixel,
         exactly the size of the image */
      for(row = 0;row < bitmap_height;row++) {
	for(col = 0;col < bitmap_width;col++) {
          if(bmp[(bitmap_height - row) * PaddedWidth]) {
            bitmap[ (row/8) * bitmap_width + col ] |= 1<<(row&7);
          }
          else {
            bitmap[ (row/8) * bitmap_width + col ] &= ~ 1<<(row&7);
          }
	}
      }

#endif
   }
   return 0; /* success */
}

#ifdef STANDALONE

/*********************************************************************
** read_next_converted_byte()
**
** Reads the next 6-pixel chunk from the 1-byte-per-pixel array,
** padding the last byte with zeros if the size is not divisible by 6.
**********************************************/
unsigned char read_next_converted_byte(void)
{
   unsigned char dest;
   unsigned int i;
   static unsigned int row = 0, col = 0;

   dest = 0;
   for(i = 0;i < 6 && col < bitmap_width;i++,col++)
   {
      if(bitmap[row * bitmap_width + col])
      {
	dest |= (unsigned char)(1 << (5-i));
      }
   }

   if(col >= bitmap_width)
   {
      col = 0;
      row++;
   }

   return dest;
}

/*********************************************************************
** convert_image()
**
** Converts the 1-byte-per-pixel array into a 6-pixel-per-byte array,
** i.e the BMP_FORMAT_VANILLA format.
**********************************************/
void convert_image(void)
{
   int newsize;
   unsigned int row, col;

   rounded_width = fh.Width/6 + ((fh.Width%6)?1:0);
   newsize = rounded_width * fh.Height;

   converted_bmp = (unsigned char *)malloc(newsize);

   for(row = 0;row < fh.Height;row++)
   {
      for(col = 0;col < rounded_width;col++)
      {
	converted_bmp[row * rounded_width + col] = read_next_converted_byte();
      }
   }
   converted_size = rounded_width * fh.Height;
}

#define COMPRESSED_ZEROS_AHEAD 0x40
#define COMPRESSED_ONES_AHEAD 0x80

/*********************************************************************
** compress_image()
**
** Compresses the BMP_FORMAT_VANILLA format with a simple RLE
** algorithm. The output is in the BMP_FORMAT_RLE format.
**********************************************/
void compress_image(void)
{
   unsigned int i, j, count;
   unsigned int index = 0;
   unsigned char val;

   compressed_bmp = (unsigned char *)malloc(converted_size);

   for(i = 0;i < converted_size;i++)
   {
      val = converted_bmp[i];

      if(val == 0|| val == 0x3f)
      {
	count = 0;
	while(count < 0x4000 && (i + count) < converted_size &&
	      converted_bmp[i+count] == val)
	{
	   count++;
	}
	if(count > 2)
	{
	   compressed_bmp[index++] = (unsigned char)
	      (((val == 0)?COMPRESSED_ZEROS_AHEAD:COMPRESSED_ONES_AHEAD) |
	      (count >> 8));
	   compressed_bmp[index++] = (unsigned char)(count & 0xff);
	}
	else
	{
	   for(j = 0;j < count;j++)
	   {
	      compressed_bmp[index++] = val;
	   }
	}
	i += count - 1;
      }
      else
      {
	compressed_bmp[index++] = val;
      }
   }

   compressed_size = index;
}

/*********************************************************************
** generate_c_source()
**
** Outputs a C source code with the converted/compressed bitmap in
** an array, accompanied by some #define's
**********************************************/
void generate_c_source(char *id, BOOL compressed)
{
   FILE *f;
   unsigned int i;
   unsigned int size;
   unsigned char *bmp;

   size = compressed?compressed_size:converted_size;
   bmp = compressed?compressed_bmp:converted_bmp;

   f = stdout;

   fprintf(f, "#define %s_WIDTH %d\n", id, rounded_width * 6);
   fprintf(f, "#define %s_HEIGHT %d\n", id, fh.Height);
   fprintf(f, "#define %s_SIZE %d\n", id, size + 6);
   if(compressed)
   {
      fprintf(f, "#define %s_ORIGINAL_SIZE %d\n", id, converted_size);
   }
   fprintf(f, "#define %s_FORMAT %s\n", id,
	      compressed?"BMP_FORMAT_RLE":"BMP_FORMAT_VANILLA");
   fprintf(f, "\nconst unsigned char bmpdata_%s[] = {", id);

   /* Header */
   fprintf(f, "\n    %s, 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,",
	compressed?"BMP_FORMAT_RLE":"BMP_FORMAT_VANILLA",
	size >> 8, size & 0xff, 2, fh.Height, rounded_width * 6);

   for(i = 0;i < size;i++)
   {
      if(i % 10 == 0)
      {
	fprintf(f, "\n    ");
      }
      fprintf(f, "0x%02x,", bmp[i]);
   }
   fprintf(f, "\n};");
}

/*********************************************************************
** generate_asm_source()
**
** Outputs an ASM source code with the converted/compressed bitmap in
** an array, the first 2 bytes describing the X and Y size
**********************************************/
void generate_asm_source(char *id, BOOL compressed)
{
   FILE *f;
   unsigned int i;
   unsigned int size;
   unsigned char *bmp;

   size = compressed?compressed_size:converted_size;
   bmp = compressed?compressed_bmp:converted_bmp;

   f = stdout;

   fprintf(f, "bmpdata_%s:\n", id);
   /* Header */
   fprintf(f, "\n    db %s, %.2xh,%.2xh,%.2xh,%.2xh,%.2xh,",
	compressed?"1":"0",
	size >> 8, size & 0xff, 2, fh.Height, rounded_width * 6);

   for(i = 0;i < size;i++)
   {
      if(i % 10 == 0)
      {
	fprintf(f, "\n   db ");
      }
      fprintf(f, "%.2xh,", bmp[i]);
   }
   fprintf(f, "\n");
}

void print_usage(void)
{
   printf("bmp2mt - Converts BMP files to MT Pro source code format\n");
   printf("build date: " __DATE__ "\n\n");
   printf("Usage: %s [-i <id>] [-c] [-a] <bitmap file>\n"
   "-i <id>     Bitmap ID (default is filename without extension)\n"
   "-c          Compress (BMP_FORMAT_RLE)\n"
   "-f          Frexx Format!!!\n"
   "-a          Assembly format source code\n", APPLICATION_NAME);
}

#pragma argsused
int main(int argc, char **argv)
{
   char *bmp_filename = NULL;
   char *id = NULL;
   char errstr[80];
   int i;

   for(i = 1;i < argc;i++)
   {
      if(argv[i][0] == '-')
      {
	switch(argv[i][1])
	{
	case 'i':   /* ID */
	   if(argv[i][2])
	   {
	      id = &argv[i][2];
	   }
	   else if(argc > i+1)
	   {
	      id = argv[i+1];
	      i++;
	   }
	   else
	   {
	      print_usage();
	      exit(1);
	   }
	   break;

	case 'c':   /* Compressed */
	   compress = true;
	   break;

	case 'a':   /* Assembly */
	   assembly = true;
	   break;

	default:
	   print_usage();
	   exit(1);
	   break;
	}
      }
      else
      {
	if(!bmp_filename)
	{
	   bmp_filename = argv[i];
	}
	else
	{
	   print_usage();
	   exit(1);
	}
      }
   }

   if(!bmp_filename)
   {
      print_usage();
      exit(1);
   }

   if(!id)
   {
      id = strdup(bmp_filename);

      for(i = 0;id[i];i++)
      {
	if(id[i] == ' ')
	{
	   id[i] = '_';
	}
	else if(id[i] == '.')
	{
	   id[i] = '\0';
	   break;
	}
	else
	{
	   id[i] = (char)toupper(id[i]);
	}
      }
   }

   read_bmp_file(bmp_filename);
   convert_image();
   if(fh.Width % 6)
   {

      sprintf(errstr, "warning - width is not divisible by 6 (%d), "
		"padding with zeros to %d\n", fh.Width, rounded_width*6);
      print_error(errstr, 0);
   }

   if(compress)
   {
      compress_image();
   }

   if(assembly)
   {
      generate_asm_source(id, compress);
   }
   else
   {
      generate_c_source(id, compress);
   }
   return 0;
}

#endif
