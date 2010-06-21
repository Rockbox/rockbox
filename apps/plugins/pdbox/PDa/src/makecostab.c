#include "m_pd.h"
#include "m_fixed.h"
#include <math.h>

#define ILOGCOSTABSIZE 15
#define ICOSTABSIZE (1<<ILOGCOSTABSIZE)

int main(int argc,char** argv)
{
    int i;
    int *fp;
    double phase, phsinc = (2. * M_PI) / ICOSTABSIZE;

    printf("#define ILOGCOSTABSIZE 15\n");
    printf("#define ICOSTABSIZE (1<<ILOGCOSTABSIZE)\n");
    printf("static t_sample cos_table[] = {");
    for (i = ICOSTABSIZE + 1,phase = 0; i--; phase += phsinc) {
	 printf("%d,",ftofix(cos(phase)));
	 //	 post("costab %f %f",cos(phase),fixtof(*fp));

    }
    printf("0};\n");

}

