#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t byte;

void xor_(byte *a, byte *b, int n);
void EncryptAES(byte *msg, byte *key, byte *c);
void DecryptAES(byte *c, byte *key, byte *m);
void Pretty(byte* b,int len,const char* label);
