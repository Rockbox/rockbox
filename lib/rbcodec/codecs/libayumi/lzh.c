#include "lzh.h"

#include <string.h>

#define BUFSIZE (1024 * 4)

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#ifndef CHAR_BIT
#define CHAR_BIT	8
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX	255
#endif

typedef ushort BITBUFTYPE;

#define BITBUFSIZ	(CHAR_BIT * sizeof(BITBUFTYPE))
#define DICBIT		13	/* 12(-lh4-) or 13(-lh5-) */
#define DICSIZ		(1U << DICBIT)
#define MAXMATCH	256	/* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD	3	/* choose optimal value */
#define NC		(UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)	/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT		9	/* $\lfloor \log_2 NC \rfloor + 1$ */
#define CODE_BIT	16	/* codeword length */

#define MAX_HASH_VAL	(3 * DICSIZ + (DICSIZ / 512 + 1) * UCHAR_MAX)

#define NP 		(DICBIT + 1)
#define NT		(CODE_BIT + 3)
#define	PBIT		4	/* smallest integer such that (1U << PBIT) > NP */
#define	TBIT		5	/* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
#define NPT		NT
#else
#define NPT		NP
#endif

uchar *m_pSrc;
int m_srcSize;
uchar *m_pDst;
int m_dstSize;

int DataIn(void *pBuffer, int nBytes);
int DataOut(void *pOut, int nBytes);

void fillbuf(int n);
ushort getbits(int n);
void init_getbits(void);
int make_table(int nchar, uchar *bitlen, int tablebits, ushort *table);
void read_pt_len(int nn, int nbit, int i_special);
void read_c_len(void);
ushort decode_c(void);
ushort decode_p(void);
void huf_decode_start(void);
void decode_start(void);
void decode(uint count, uchar buffer[]);

int fillbufsize;
uchar buf[BUFSIZE];
uchar outbuf[DICSIZ];
ushort left[2 * NC - 1];
ushort right[2 * NC - 1];
BITBUFTYPE bitbuf;
uint subbitbuf;
int bitcount;
int decode_j;		/* remaining bytes to copy */
uchar c_len[NC];
uchar pt_len[NPT];
uint blocksize;
ushort c_table[4096];
ushort pt_table[256];
int with_error;

uint fillbuf_i;		/* NOTE: these ones are not initialized at constructor time but inside the fillbuf and decode func. */
uint decode_i;

/* Additions */

int DataIn(void *pBuffer, int nBytes)
{
  const int np = (nBytes <= m_srcSize) ? nBytes : m_srcSize;
  if (np > 0) {
    memcpy(pBuffer, m_pSrc, np);
    m_pSrc += np;
    m_srcSize -= np;
  }
  return np;
}

int DataOut(void *pBuffer, int nBytes)
{
  const int np = (nBytes <= m_dstSize) ? nBytes : m_dstSize;
  if (np > 0) {
    memcpy(m_pDst, pBuffer, np);
    m_pDst += np;
    m_dstSize -= np;
  }
  return np;
}

/* io.c */

/* Shift bitbuf n bits left, read n bits */
void fillbuf(int n)
{
  bitbuf = (bitbuf << n) & 0xffff;
  while (n > bitcount) {
    bitbuf |= subbitbuf << (n -= bitcount);
    if (fillbufsize == 0) {
      fillbuf_i = 0;
      fillbufsize = DataIn(buf, BUFSIZE - 32);
    }
    if (fillbufsize > 0)
      fillbufsize--, subbitbuf = buf[fillbuf_i++];
    else
      subbitbuf = 0;
    bitcount = CHAR_BIT;
  }
  bitbuf |= subbitbuf >> (bitcount -= n);
}

ushort getbits(int n)
{
  ushort x;
  x = bitbuf >> (BITBUFSIZ - n);
  fillbuf(n);
  return x;
}

void init_getbits(void)
{
  bitbuf = 0;
  subbitbuf = 0;
  bitcount = 0;
  fillbuf(BITBUFSIZ);
}

/* maketbl.c */

int make_table(int nchar, uchar * bitlen, int tablebits, ushort * table)
{
  ushort count[17], weight[17], start[18], *p;
  uint jutbits, avail, mask;
  int i, ch, len, nextcode;

  for (i = 1; i <= 16; i++)
    count[i] = 0;
  for (i = 0; i < nchar; i++)
    count[bitlen[i]]++;

  start[1] = 0;
  for (i = 1; i <= 16; i++)
    start[i + 1] = start[i] + (count[i] << (16 - i));
  if (start[17] != (ushort) (1U << 16))
    return (1);			/* error: bad table */

  jutbits = 16 - tablebits;
  for (i = 1; i <= tablebits; i++) {
    start[i] >>= jutbits;
    weight[i] = 1U << (tablebits - i);
  }
  while (i <= 16) {
    weight[i] = 1U << (16 - i);
    i++;
  }

  i = start[tablebits + 1] >> jutbits;
  if (i != (ushort) (1U << 16)) {
    int k = 1U << tablebits;
    while (i != k)
      table[i++] = 0;
  }

  avail = nchar;
  mask = 1U << (15 - tablebits);
  for (ch = 0; ch < nchar; ch++) {
    if ((len = bitlen[ch]) == 0)
      continue;
    nextcode = start[len] + weight[len];
    if (len <= tablebits) {
      for (i = start[len]; i < nextcode; i++)
	table[i] = ch;
    } else {
      uint k = start[len];
      p = &table[k >> jutbits];
      i = len - tablebits;
      while (i != 0) {
	if (*p == 0) {
	  right[avail] = left[avail] = 0;
	  *p = avail++;
	}
	if (k & mask)
	  p = &right[*p];
	else
	  p = &left[*p];
	k <<= 1;
	i--;
      }
      *p = ch;
    }
    start[len] = nextcode;
  }
  return (0);
}

/* huf.c */

void read_pt_len(int nn, int nbit, int i_special)
{
  int i, n;
  short c;
  ushort mask;

  n = getbits(nbit);
  if (n == 0) {
    c = getbits(nbit);
    for (i = 0; i < nn; i++)
      pt_len[i] = 0;
    for (i = 0; i < 256; i++)
      pt_table[i] = c;
  } else {
    i = 0;
    while (i < n) {
      c = bitbuf >> (BITBUFSIZ - 3);
      if (c == 7) {
	mask = 1U << (BITBUFSIZ - 1 - 3);
	while (mask & bitbuf) {
	  mask >>= 1;
	  c++;
	}
      }
      fillbuf((c < 7) ? 3 : c - 3);
      pt_len[i++] = (unsigned char) (c);
      if (i == i_special) {
	c = getbits(2);
	while (--c >= 0)
	  pt_len[i++] = 0;
      }
    }
    while (i < nn)
      pt_len[i++] = 0;
    make_table(nn, pt_len, 8, pt_table);
  }
}

void read_c_len(void)
{
  short i, c, n;
  ushort mask;

  n = getbits(CBIT);
  if (n == 0) {
    c = getbits(CBIT);
    for (i = 0; i < NC; i++)
      c_len[i] = 0;
    for (i = 0; i < 4096; i++)
      c_table[i] = c;
  } else {
    i = 0;
    while (i < n) {
      c = pt_table[bitbuf >> (BITBUFSIZ - 8)];
      if (c >= NT) {
	mask = 1U << (BITBUFSIZ - 1 - 8);
	do {
	  if (bitbuf & mask)
	    c = right[c];
	  else
	    c = left[c];
	  mask >>= 1;
	} while (c >= NT);
      }
      fillbuf(pt_len[c]);
      if (c <= 2) {
	if (c == 0)
	  c = 1;
	else if (c == 1)
	  c = getbits(4) + 3;
	else
	  c = getbits(CBIT) + 20;
	while (--c >= 0)
	  c_len[i++] = 0;
      } else
	c_len[i++] = c - 2;
    }
    while (i < NC)
      c_len[i++] = 0;
    make_table(NC, c_len, 12, c_table);
  }
}

ushort decode_c(void)
{
  ushort j, mask;

  if (blocksize == 0) {
    blocksize = getbits(16);
    read_pt_len(NT, TBIT, 3);
    read_c_len();
    read_pt_len(NP, PBIT, -1);
  }
  blocksize--;
  j = c_table[bitbuf >> (BITBUFSIZ - 12)];
  if (j >= NC) {
    mask = 1U << (BITBUFSIZ - 1 - 12);
    do {
      if (bitbuf & mask)
	j = right[j];
      else
	j = left[j];
      mask >>= 1;
    }
    while (j >= NC);
  }
  fillbuf(c_len[j]);
  return j;
}

ushort decode_p(void)
{
  ushort j, mask;

  j = pt_table[bitbuf >> (BITBUFSIZ - 8)];
  if (j >= NP) {
    mask = 1U << (BITBUFSIZ - 1 - 8);
    do {
      if (bitbuf & mask)
	j = right[j];
      else
	j = left[j];
      mask >>= 1;
    } while (j >= NP);
  }
  fillbuf(pt_len[j]);
  if (j != 0)
    j = (1U << (j - 1)) + getbits(j - 1);
  return j;
}

void huf_decode_start(void)
{
  init_getbits();
  blocksize = 0;
}

/* decode.c */

void decode_start(void)
{
  fillbufsize = 0;
  huf_decode_start();
  decode_j = 0;
}

/*
 * The calling function must keep the number of bytes to be processed.  This
 * function decodes either 'count' bytes or 'DICSIZ' bytes, whichever is
 * smaller, into the array 'buffer[]' of size 'DICSIZ' or more. Call
 * decode_start() once for each new file before calling this function.
 */
void decode(uint count, uchar buffer[])
{
  uint r, c;

  r = 0;
  while (--decode_j >= 0) {
    buffer[r] = buffer[decode_i];
    decode_i = (decode_i + 1) & (DICSIZ - 1);
    if (++r == count)
      return;
  }
  for (;;) {
    c = decode_c();
    if (c <= UCHAR_MAX) {
      buffer[r] = c;
      if (++r == count)
	return;
    } else {
      decode_j = c - (UCHAR_MAX + 1 - THRESHOLD);
      decode_i = (r - decode_p() - 1) & (DICSIZ - 1);
      while (--decode_j >= 0) {
	buffer[r] = buffer[decode_i];
	decode_i = (decode_i + 1) & (DICSIZ - 1);
	if (++r == count)
	  return;
      }
    }
  }
}

int LzUnpack(void *pSrc, int srcSize, void *pDst, int dstSize)
{
  with_error = 0;

  m_pSrc = (uchar *) pSrc;
  m_srcSize = srcSize;
  m_pDst = (uchar *) pDst;
  m_dstSize = dstSize;

  decode_start();

  unsigned int origsize = dstSize;
  while (origsize != 0) {
    int n = (uint) ((origsize > DICSIZ) ? DICSIZ : origsize);
    decode(n, outbuf);
    if (with_error)
      break;

    DataOut(outbuf, n);
    origsize -= n;
    if (with_error)
      break;
  }

  return (with_error);
}
