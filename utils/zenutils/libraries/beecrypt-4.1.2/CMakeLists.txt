PROJECT(beecrypt)

# source files for beecrypt
SET(beecrypt_srcs
  beecrypt/blockmode.c
  beecrypt/blowfish.c
  beecrypt/endianness.c
  beecrypt/hmac.c
  beecrypt/hmacsha1.c
  beecrypt/mp.c
  beecrypt/sha1.c
)

ADD_LIBRARY(beecrypt ${beecrypt_srcs})
