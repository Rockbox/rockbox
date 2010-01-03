#include <stdio.h>

int main(int argc, char *argv[])
{
    FILE *f;
    int i;
    unsigned char buf[128000];

    f = fopen("mp.mp3", "r");
    
    if(f)
    {
    if(fread(buf, 1, 128000, f) < 128000)
    {
        fprintf(stderr, "FAN!\n");
        exit(1);
    }

    printf("int mp3datalen = 128000;\n");
    printf("unsigned char mp3data[128000] =\n{");
    for(i = 0;i < 128000;i++)
    {
        if(i % 8 == 0)
        {
        printf("\n");
        }
        printf("0x%02x, ", buf[i]);
    }
    printf("};\n");
    }
}
