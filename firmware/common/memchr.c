/*
FUNCTION
	<<memchr>>---search for character in memory

INDEX
	memchr

ANSI_SYNOPSIS
	#include <string.h>
	void * memchr(const void *<[s1]>, int <[c]>, size_t <[n]>);

TRAD_SYNOPSIS
	#include <string.h>
	void * memchr(<[s1]>, <[c]>, <[n]>);
	void *<[string]>;
	int *<[c]>;
	size_t *<[n]>;

DESCRIPTION
	This function scans the first <[n]> bytes of the memory pointed
	to by <[s1]> for the character <[c]> (converted to a char).

RETURNS
	Returns a pointer to the matching byte, or a null pointer if
	<[c]> does not occur in <[s1]>.

PORTABILITY
<<memchr>> is ANSI C.

<<memchr>> requires no supporting OS subroutines.

QUICKREF
	memchr ansi pure
*/

#include <string.h>
#include <limits.h>

/* Nonzero if X is not aligned on a "long" boundary.  */
#define UNALIGNED(X) ((long)X & (sizeof (long) - 1))

/* How many bytes are loaded each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof (long))

#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

/* DETECTCHAR returns nonzero if (long)X contains the byte used 
   to fill (long)MASK. */
#define DETECTCHAR(X,MASK) (DETECTNULL(X ^ MASK))

void *
_DEFUN (memchr, (s1, i, n),
	_CONST void *s1 _AND
	int i _AND size_t n)
{
  _CONST unsigned char *s = (_CONST unsigned char *)s1;
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  unsigned char c = (unsigned char)i;

  while (n-- > 0)
    {
      if (*s == c)
	{
          return (void *)s;
	}
      s++;
    }

  return NULL;
#else
  unsigned char c = (unsigned char)i;
  unsigned long mask,j;
  unsigned long *aligned_addr;

  if (!UNALIGNED (s))
    {
      mask = 0;
      for (j = 0; j < LBLOCKSIZE; j++)
        mask = (mask << 8) | c;

      aligned_addr = (unsigned long*)s;
      while ((!DETECTCHAR (*aligned_addr, mask)) && (n>LBLOCKSIZE))
	{
          aligned_addr++;
	  n -= LBLOCKSIZE;
	}

      /* The block of bytes currently pointed to by aligned_addr
         may contain the target character or there may be less than
	 LBLOCKSIZE bytes left to search. We check the last few
	 bytes using the bytewise search.  */

      s = (unsigned char*)aligned_addr;
    }

  while (n-- > 0)
    {
      if (*s == c)
	{
          return (void *)s;
	}
      s++;
    }

  return NULL;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
