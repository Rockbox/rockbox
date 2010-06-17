#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define ULONG  uint32_t
#define USHORT uint16_t
#define UCHAR  uint8_t

ULONG isdata[1000000]; /* each bit defines one byte as: code=0, data=1 */

extern void dis_asm(ULONG off, ULONG val, char *stg);

int static inline le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

int main(int argc, char **argv)
{
  FILE   *in, *out;
  char   *ptr, stg[256];
  unsigned char buf[4];
  ULONG  pos, sz, val, loop;
  int    offset, offset1;
  USHORT regid;

  if(argc == 1 || strcmp(argv[1], "--help") == 0)
  { printf("Usage: arm_disass [input file]\n");
    printf(" disassembles input file to 'disasm.txt'\n");
    exit(-1);
  }

  in = fopen(argv[1], "rb");
  if(in == NULL)
  { printf("Cannot open %s", argv[1]);
    exit(-1);
  }

  out = fopen("disasm.txt", "w");
  if(out == NULL) exit(-1);

  fseek(in, 0, SEEK_END);
  sz = ftell(in);

  /* first loop only sets data/code tags */
  for(loop=0; loop<2; loop++)
  {
    for(pos=0; pos<sz; pos+=4)
    {
      /* clear disassembler string start */
      memset(stg, 0, 40);
      /* read next code dword */
      fseek(in, pos, SEEK_SET);
      fread(buf, 1, 4, in);
      
      val = le2int(buf);

      /* check for data tag set: if 1 byte out of 4 is marked => assume data */
      if((isdata[pos>>5] & (0xf << (pos & 31))) || (val & 0xffff0000) == 0)
      {
        sprintf(stg, "%6x:	%08x", pos, val);
      }
      else
      {
        dis_asm(pos, val, stg);

        /* check for instant mov operation */
        if(memcmp(stg+17, "mov ", 4) == 0 && (ptr=strstr(stg, "0x")) != NULL)
        {
          regid = *(USHORT*)(stg+22);

          sscanf(ptr+2, "%x", &offset);
          if(ptr[-1] == '-')
            offset = -offset;
        }
        else
        /* check for add/sub operation */
        if((ptr=strstr(stg, "0x")) != NULL
         && (memcmp(stg+17, "add ", 4) == 0 || memcmp(stg+17, "sub ", 4) == 0))
        {
          if(regid == *(USHORT*)(stg+22) && regid == *(USHORT*)(stg+26))
          {
            sscanf(ptr+2, "%x", &offset1);
            if(ptr[-1] == '-')
              offset1 = -offset1;

            if(memcmp(stg+17, "add ", 4) == 0) offset += offset1;
            else                               offset -= offset1;

            /* add result to disassembler string */
            sprintf(stg+strlen(stg), " <- 0x%x", offset);
          }
          else
            regid = 0;
        }
        else
          regid = 0;

        /* check for const data */
        if(memcmp(stg+26, "[pc, ", 5) == 0 && (ptr=strstr(stg, "0x")) != NULL)
        {
          sscanf(ptr+2, "%x", &offset);
          if(ptr[-1] == '-')
            offset = -offset;

          /* add data tag */
          isdata[(pos+offset+8)>>5] |= 1 << ((pos+offset+8) & 31);

          /* add const data to disassembler string */
          fseek(in, pos+offset+8, SEEK_SET);
          fread(&buf, 1, 4, in);
          offset = le2int(buf);
          
          sprintf(stg+strlen(stg), " <- 0x%x", offset);
        }
      }
    
      /* remove trailing spaces */
      while(stg[strlen(stg)-1] == 32)
        stg[strlen(stg)-1] = 0;

      if(loop == 1)
        fprintf(out, "%s\n", stg);
    }
  }

  fclose(in);
  return 0;
}
