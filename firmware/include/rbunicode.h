/*   Some conversion functions for handling UTF-8
 *
 *   copyright Marcoen Hirschberg (2004,2005)
 *
 *   I got all the info from:
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
 *   and
 *   http://en.wikipedia.org/wiki/Unicode
 */

#define CODEPAGE_DIR    "/.rockbox/codepages"

#define MAX_CP_TABLE_SIZE    32768

#define MASK   0xC0 /* 11000000 */
#define COMP   0x80 /* 10x      */

extern int codepage;

/* Encode a UCS value as UTF-8 and return a pointer after this UTF-8 char. */
unsigned char* utf8encode(unsigned long ucs, unsigned char *utf8);
unsigned char* iso_decode(const unsigned char *latin1, unsigned char *utf8, int cp, int count);
unsigned char* utf16LEdecode(const unsigned char *utf16, unsigned char *utf8, unsigned int count);
unsigned char* utf16BEdecode(const unsigned char *utf16, unsigned char *utf8, unsigned int count);
unsigned char* utf16decode(const unsigned char *utf16, unsigned char *utf8, unsigned int count);
unsigned long utf8length(const unsigned char *utf8);
const unsigned char* utf8decode(const unsigned char *utf8, unsigned short *ucs);
void set_codepage(int cp);
int utf8seek(const unsigned char* utf8, int offset);
