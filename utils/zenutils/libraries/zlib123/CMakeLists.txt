PROJECT(zlib)

# source files for zlib
SET(zlib_src
  zlib/adler32.c
  zlib/compress.c
  zlib/crc32.c
  zlib/deflate.c
  zlib/gzio.c
  zlib/infback.c
  zlib/inffast.c
  zlib/inflate.c
  zlib/inftrees.c
  zlib/trees.c
  zlib/uncompr.c
  zlib/zutil.c
)

ADD_LIBRARY(zlib ${zlib_src})
