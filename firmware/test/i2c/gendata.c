#include <stdio.h>

int main(int argc, char *argv[])
{
    FILE *f;
    int i;
    unsigned char buf[64000];

    f = fopen("mp.mp3", "r");
    
    if(f)
    {
	if(fread(buf, 1, 64000, f) < 64000)
	{
	    fprintf(stderr, "FAN!\n");
	    exit(1);
	}

	printf("int mp3datalen = 64000;\n");
	printf("unsigned char mp3data[64000] =\n{");
	for(i = 0;i < 64000;i++)
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
