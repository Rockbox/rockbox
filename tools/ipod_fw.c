/*
 * make_fw.c - iPodLinux loader installer
 * 
 * Copyright (C) 2003 Daniel Palffy
 *
 * based on Bernard Leach's patch_fw.c
 * Copyright (C) 2002 Bernard Leach
 * big endian support added 2003 Steven Lucy
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef __WIN32__
#include "getopt.c"
#endif

#define TBL 0x4200

/* Some firmwares have padding becore the actual image.  */
#define IMAGE_PADDING ((fw_version == 3) ? 0x200 : 0)
#define FIRST_OFFSET (TBL + 0x200 + IMAGE_PADDING)

int be;
unsigned short fw_version = 2;

typedef struct _image {
    char type[4];		/* '' */
    unsigned id;		/* */
    char pad1[4];		/* 0000 0000 */
    unsigned devOffset;		/* byte offset of start of image code */
    unsigned len;		/* length in bytes of image */
    unsigned addr;		/* load address */
    unsigned entryOffset;	/* execution start within image */
    unsigned chksum;		/* checksum for image */
    unsigned vers;		/* image version */
    unsigned loadAddr;		/* load address for image */
} image_t;

static char *apple_copyright = "{{~~  /-----\\   {{~~ /       \\  {{~~|         | {{~~| S T O P | {{~~|         | {{~~ \\       /  {{~~  \\-----/   Copyright(C) 2001 Apple Computer, Inc.---------------------------------------------------------------------------------------------------------";

unsigned
switch_32(unsigned l)
{
    if (be)
	return ((l & 0xff) << 24)
	    | ((l & 0xff00) << 8)
	    | ((l & 0xff0000) >> 8)
	    | ((l & 0xff000000) >> 24);
    return l;
}

unsigned short
switch_16(unsigned short s)
{
	if (be) {
		return ((s & 0xff) << 8) | ((s & 0xff00) >> 8);
	} else {
		return s;
	}
}

void
switch_endian(image_t *image)
{
   if (be) {
	image->id = switch_32(image->id);
	image->devOffset = switch_32(image->devOffset);
	image->len = switch_32(image->len);
	image->addr = switch_32(image->addr);
	image->entryOffset = switch_32(image->entryOffset);
	image->chksum = switch_32(image->chksum);
	image->vers = switch_32(image->vers);
	image->loadAddr = switch_32(image->loadAddr);
    }
}

void
print_image(image_t *image, const char *head)
{
    printf("%stype: '%s' id: 0x%4x len: 0x%4x addr: 0x%4x vers: 0x%8x\r\n", 
	    head, image->type, image->id, image->len, image->addr, image->vers);
    printf("devOffset: 0x%08X entryOffset: 0x%08X "
	    "loadAddr: 0x%08X chksum: 0x%08X\n", 
	    image->devOffset, image->entryOffset, 
	    image->loadAddr, image->chksum);
}

void
usage()
{
    printf("Usage: patch_fw [-h]\n"
	   "       patch_fw [-v] -o outfile -e img_no fw_file\n"   
	   "       patch_fw [-v] -g gen [-r rev] -o outfile [-i img_from_-e]* [-l raw_img]* ldr_img\n\n"
	   "  -g:    set target ipod generation, valid options are: 1g, 2g, 3g\n"
	   "         4g, 5g, scroll, touch, dock, mini, photo, color, nano and video\n"
	   "  -e:    extract the image at img_no in boot table to outfile\n"
	   "         fw_file is an original firmware image\n"
	   "         the original firmware has the sw at 0, and a flash updater at 1\n"
	   "  -i|-l: create new image to outfile\n"
	   "         up to 5 images, any of -i or -l allowed\n"
	   "         -i: image extracted with -e, load and entry address preserved\n"
	   "         -l: raw image, loaded to 0x28000000, entry at 0x00000000\n"
	   "	     -r: set master revision to rev (for example 210 for 2.10)\n"
	   "             may be needed if newest -e img is not the same as the flash rev\n"
	   "         ldr_img is the iPodLinux loader binary.\n"
	   "         first image is loaded by default, 2., 3., 4. or 5. loaded if\n"
	   "         rew, menu, play or ff is hold while booting\n\n"
	   "  -v: verbose\n\n"
	   " This program is used to create a bootable ipod image.\n\n");
}

/* read len bytes from the beginning of s, 
 * calculate checksum, and 
 * if (d) copy to current offset in d */
unsigned
copysum(FILE *s, FILE *d, unsigned len, unsigned off)
{
    unsigned char temp;
    unsigned sum = 0;
    unsigned i;
    if (fseek(s, off, SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return -1;
    }
    for (i = 0; i < len; i++) {
	if (fread(&temp, 1, 1, s) != 1) {
	    fprintf(stderr, "fread failed: %s\n", strerror(errno));
	    return -1;
	}
	sum = sum + (temp & 0xff);
	if (d) 
	    if (fwrite(&temp, 1, 1, d) != 1) {
		fprintf(stderr, "fwrite failed: %s\n", strerror(errno));
		return -1;
	    }
    }
    return sum;
}

/* load the boot entry from 
 * boot table at offset, 
 * entry number entry
 * file fw */
int 
load_entry(image_t *image, FILE *fw, unsigned offset, int entry)
{
    if (fseek(fw, offset + entry * sizeof(image_t), SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return -1;
    }
    if (fread(image, sizeof(image_t), 1, fw) != 1) {
	fprintf(stderr, "fread failed: %s\n", strerror(errno));
	return -1;
    }
    switch_endian(image);
    return 0;
}

/* store the boot entry to
 * boot table at offset, 
 * entry number entry
 * file fw */
int 
write_entry(image_t *image, FILE *fw, unsigned offset, int entry)
{
    if (fseek(fw, offset + entry * sizeof(image_t), SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return -1;
    }
    switch_endian(image);
    if (fwrite(image, sizeof(image_t), 1, fw) != 1) {
	fprintf(stderr, "fwrite failed: %s\n", strerror(errno));
	switch_endian(image);
	return -1;
    }
    switch_endian(image);
    return 0;
}

/* extract a single image from the fw 
 * the first 40 bytes contain a boot table entry, 
 * padded to one block (512 bytes */
int 
extract(FILE *f, int idx, FILE *out)
{
    image_t *image;
    unsigned char buf[512];
    unsigned off;

    fseek(f, 0x100 + 10, SEEK_SET);
    fread(&fw_version, sizeof(fw_version), 1, f);
    fw_version = switch_16(fw_version);

    image = (image_t *)buf;
    if (load_entry(image, f, TBL, idx) == -1)
	return -1;
    off = image->devOffset + IMAGE_PADDING;

    if (fseek(f, off, SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return -1;
    }
    if (write_entry(image, out, 0x0, 0) == -1)
	return -1;
    if (fseek(out, 512, SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return -1;
    }
    if (copysum(f, out, image->len, off) == -1)
	return -1;
		
    return 0;
}

/* return the size of f */
unsigned 
lengthof(FILE *f)
{
    unsigned ret;

    if (fseek(f, 0, SEEK_END) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return -1;
    }
    if ((ret = ftell(f)) == -1) {
	fprintf(stderr, "ftell failed: %s\n", strerror(errno));
	return -1;
    }
    return ret;
}

void
test_endian(void)
{
    char ch[4] = { '\0', '\1', '\2', '\3' };
    unsigned i = 0x00010203;

    if (*((int *)ch) == i) 
	be = 1;
    else
	be = 0;
    return;
}

int
main(int argc, char **argv)
{
    int c;
    int verbose = 0, i, ext = 0;
    FILE *in = NULL, *out = NULL;
    char gen_set = 0;
    image_t images[5];
    image_t image = {
	{ '!', 'A', 'T', 'A' },	    // magic
	0x6f736f73,		    // id
	{ '\0', '\0', '\0', '\0' }, // pad
	0x4400,			    // devOffset
	0,			    // len
	0x28000000,		    // addr
	0,			    // entry
	0,			    // chksum
	0,			    // vers
	0xffffffff		    // loadAddr
    };
    int images_done = 0;
    unsigned version = 0, offset = 0, len = 0;
    int needs_rcsc = 0;
    
    test_endian();
    
    /* parse options */
    opterr = 0;
    while ((c = getopt(argc, argv, "3hve:o:i:l:r:g:")) != -1)
	switch (c) {
	    case 'h':
		if (verbose || in || out || images_done || ext) {
		    fprintf(stderr, 
			    "-[?h] is exclusive with other arguments\n");
		    usage();
		    return 1;
		}
		usage();
		return 0;
	    case 'v':
		if (verbose++)
		    fprintf(stderr, "Warning: multiple -v options specified\n");
		break;
	    case '3':
		gen_set = 1;
		fw_version = 3;
		image.addr = 0x10000000;
		break;
	    case 'g':
		gen_set = 1;
		if ((strcasecmp(optarg, "4g") == 0) ||
			(strcasecmp(optarg, "mini") == 0) ||
			(strcasecmp(optarg, "nano") == 0) ||
			(strcasecmp(optarg, "photo") == 0) ||
			(strcasecmp(optarg, "color") == 0) ||
			(strcasecmp(optarg, "video") == 0) ||
			(strcasecmp(optarg, "5g") == 0)) {
		    fw_version = 3;
		    image.addr = 0x10000000;
		    if ((strcasecmp(optarg, "5g") == 0) || (strcasecmp(optarg, "video") == 0)) {
			    needs_rcsc = 1;
		    }
		}
		else if ((strcasecmp(optarg, "1g") != 0) &&
			(strcasecmp(optarg, "2g") != 0) &&
			(strcasecmp(optarg, "3g") != 0) &&
			(strcasecmp(optarg, "scroll") != 0) &&
			(strcasecmp(optarg, "touch") != 0) &&
			(strcasecmp(optarg, "dock") != 0)) {
		    fprintf(stderr, "%s: bad gen. Valid options are: 1g, 2g,"
			    " 3g, 4g, 5g, scroll, touch, dock, mini, nano,"
			    " photo, color, and video\n", optarg);
		    return 1;
		}
		break;
	    case 'o':
		if (out) {
		    fprintf(stderr, "output already opened\n");
		    usage();
		    return 1;
		}
		if ((out = fopen(optarg, "wb+")) == NULL) {
		    fprintf(stderr, "Cannot open output file %s\n", optarg);
		    return 1;
		}
		break;
	    case 'e':
		if (!out || images_done || ext) {
		    usage();
		    return 1;
		}
		    
		ext = atoi(optarg) + 1;
		break;
	    case 'i':
		if (!out || ext) {
		    usage();
		    return 1;
		}
		if (images_done == 5) {
		    fprintf(stderr, "Only 5 images supported\n");
		    return 1;
		}
		if ((in = fopen(optarg, "rb")) == NULL) {
		    fprintf(stderr, "Cannot open firmware image file %s\n", optarg);
		    return 1;
		}
		if (load_entry(images + images_done, in, 0, 0) == -1)
		    return 1;
		if (!offset) offset = FIRST_OFFSET;
		else offset = (offset + 0x1ff) & ~0x1ff;
		images[images_done].devOffset = offset;
		if (fseek(out, offset, SEEK_SET) == -1) {
		    fprintf(stderr, "fseek failed: %s\n", strerror(errno));
		    return 1;
		}
		if ((images[images_done].chksum = copysum(in, out, 
			images[images_done].len, 0x200)) == -1)
		    return 1;
		offset += images[images_done].len;
		if (verbose) print_image(images + images_done, "Apple image added: ");
		images_done++;
		fclose(in);
		break;
	    case 'l':
		if (!out || ext) {
		    usage();
		    return 1;
		}
		if (images_done == 5) {
		    fprintf(stderr, "Only 5 images supported\n");
		    return 1;
		}
		if ((in = fopen(optarg, "rb")) == NULL) {
		    fprintf(stderr, "Cannot open linux image file %s\n", optarg);
		    return 1;
		}
		if (!offset) offset = FIRST_OFFSET;
		else offset = (offset + 0x1ff) & ~0x1ff;
		images[images_done] = image;
		images[images_done].devOffset = offset;
		if ((images[images_done].len = lengthof(in)) == -1)
		    return 1;
		if (fseek(out, offset, SEEK_SET) == -1) {
		    fprintf(stderr, "fseek failed: %s\n", strerror(errno));
		    return 1;
		}
		if ((images[images_done].chksum = copysum(in, out, 
			images[images_done].len, 0)) == -1)
		    return 1;
		offset += images[images_done].len;
		if (verbose) print_image(images + images_done, "Linux image added: ");
		images_done++;
		fclose(in);
		break;
	    case 'r':
		if (ext) {
		    usage();
		    return 1;
		}
		version = strtol(optarg, NULL, 16);
		break;
	    case '?':
		fprintf(stderr, "invalid option -%c specified\n", optopt);
		usage();
		return 1;
	    case ':':
		fprintf(stderr, "option -%c needs an argument\n", optopt);
		usage();
		return 1;
	}

    if (argc - optind != 1) {
	usage();
	return 1;
    }
 
    if (ext) {
	if ((in = fopen(argv[optind], "rb")) == NULL) {
	    fprintf(stderr, "Cannot open firmware image file %s\n", argv[optind]);
	    return 1;
	}
	if (extract(in, ext-1, out) == -1) return 1;
	fclose(in); fclose(out);
	return 0;
    }
    else if (!gen_set) {
	usage();
	return 1;
    }

    printf("Generating firmware image compatible with ");
    if (fw_version == 3) {
	printf("iPod mini, 4g and iPod photo...\n");
    } else {
	printf("1g, 2g and 3g iPods...\n");
    }

    if (!images_done) {
	fprintf(stderr, "no images specified!\n");
	return 1;
    }
    if ((in = fopen(argv[optind], "rb")) == NULL) {
	fprintf(stderr, "Cannot open loader image file %s\n", argv[optind]);
	return 1;
    }
    offset = (offset + 0x1ff) & ~0x1ff;
    if ((len = lengthof(in)) == -1)
	return 1;
    if (fseek(out, offset, SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return 1;
    }
    if (copysum(in, out, len, 0) == -1)
	return 1;
    for (i=0; i < images_done; i++) {
	if (images[i].vers > image.vers) image.vers = images[i].vers;
	if (write_entry(images+i, out, offset+0x0100, i) == -1)
	    return 1;
    }
    if (version) image.vers = version;
    image.len = offset + len - FIRST_OFFSET;
    image.entryOffset = offset - FIRST_OFFSET;
    if ((image.chksum = copysum(out, NULL, image.len, FIRST_OFFSET)) == -1)
	return 1;
    if (fseek(out, 0x0, SEEK_SET) == -1) {
	fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	return 1;
    }
    if (fwrite(apple_copyright, 0x100, 1, out) != 1) {
	fprintf(stderr, "fwrite failed: %s\n", strerror(errno));
	return 1;
    }
    version = switch_32(0x5b68695d); /* magic */
    if (fwrite(&version, 4, 1, out) != 1) {
	fprintf(stderr, "fwrite failed: %s\n", strerror(errno));
	return 1;
    }
    version = switch_32(0x00004000); /* magic */
    if (fwrite(&version, 4, 1, out) != 1) {
	fprintf(stderr, "fwrite failed: %s\n", strerror(errno));
	return 1;
    }
    if (fw_version == 3) {
      version = switch_32(0x0003010c); /* magic */
    } else {
      version = switch_32(0x0002010c); /* magic */
    }

    if (fwrite(&version, 4, 1, out) != 1) {
	fprintf(stderr, "fwrite failed: %s\n", strerror(errno));
	return 1;
    }
    if (write_entry(&image, out, TBL, 0) == -1)
	return 1;
    if (verbose) print_image(&image, "Master image: ");

    if (needs_rcsc) {
	image_t rsrc;

	if ((in = fopen("apple_sw_5g_rcsc.bin", "rb")) == NULL) {
	    fprintf(stderr, "Cannot open firmware image file %s\n", "apple_sw_5g_rcsc.bin");
	    return 1;
	}
	if (load_entry(&rsrc, in, 0, 0) == -1) {
	    return 1;
	}
	rsrc.devOffset = image.devOffset + image.len;
	rsrc.devOffset = ((rsrc.devOffset + 0x1ff) & ~0x1ff) + 0x200;
	if (fseek(out, rsrc.devOffset + IMAGE_PADDING, SEEK_SET) == -1) {
	    fprintf(stderr, "fseek failed: %s\n", strerror(errno));
	    return 1;
    	}
	if ((rsrc.chksum = copysum(in, out, rsrc.len, 0x200)) == -1) {
	    return 1;
	}
	fclose(in);

	if (write_entry(&rsrc, out, TBL, 1) == -1) {
	    return 1;
	}

        if (verbose) print_image(&rsrc, "rsrc image: ");
    }

    return 0;
}

