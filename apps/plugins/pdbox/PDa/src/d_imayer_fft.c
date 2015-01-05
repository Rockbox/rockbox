/*
** Algorithm: complex multiplication
** 
** Performance: Bad accuracy, great speed.
** 
** The simplest, way of generating trig values.  Note, this method is
** not stable, and errors accumulate rapidly.
** 
** Computation: 2 *, 1 + per value.
*/

#include "m_pd.h"

#include "m_fixed.h"


#define TRIG_FAST

#ifdef TRIG_FAST
char mtrig_algorithm[] = "Simple";
#define TRIG_VARS                                        \
      t_fixed t_c,t_s;
#define TRIG_INIT(k,c,s)                                 \
    {                                                    \
     t_c  = fcostab[k];                                   \
     t_s  = fsintab[k];                                   \
     c    = itofix(1);                                   \
     s    = 0;                                           \
    }
#define TRIG_NEXT(k,c,s)                                 \
    {                                                    \
     t_fixed   t = c;                                    \
     c   = mult(t,t_c) - mult(s,t_s);              \
     s   = mult(t,t_s) + mult(s,t_c);              \
    }
#define TRIG_23(k,c1,s1,c2,s2,c3,s3)                     \
        {                                                \
         c2 = mult(c1,c1) - mult(s1,s1);           \
         s2 = (mult(c1,s1)<<2);                    \
         c3 = mult(c2,c1) - mult(s2,s1);           \
         s3 = mult(c2,s1) + mult(s2,c1);           \
        }
#endif
#define TRIG_RESET(k,c,s)

/*
** Algorithm: O. Buneman's trig generator.
** 
** Performance: Good accuracy, mediocre speed.
** 
**   Manipulates a log(n) table to stably create n evenly spaced trig
**   values. The newly generated values lay halfway between two
**   known values, and are calculated by appropriatelly scaling the
**   average of the known trig values appropriatelly according to the
**   angle between them.  This process is inherently stable; and is
**   about as accurate as most trig library functions.  The errors
**   caused by this code segment are primarily due to the less stable
**   method to calculate values for 2t and s 3t. Note: I believe this
**   algorithm is patented(!), see readme file for more details.
**
** Computation: 1 +, 1 *, much integer math,  per trig value
**
*/

#ifdef TRIG_GOOD
#define TRIG_VARS                                                \
      int t_lam=0;                                               \
      double coswrk[TRIG_TABLE_SIZE],sinwrk[TRIG_TABLE_SIZE];
#define TRIG_INIT(k,c,s)                                         \
     {                                                           \
      int i;                                                     \
      for (i=0 ; i<=k ; i++)                                     \
          {coswrk[i]=fcostab[i];sinwrk[i]=fsintab[i];}             \
      t_lam = 0;                                                 \
      c = 1;                                                     \
      s = 0;                                                     \
     }
#define TRIG_NEXT(k,c,s)                                         \
     {                                                           \
         int i,j;                                                \
         (t_lam)++;                                              \
         for (i=0 ; !((1<<i)&t_lam) ; i++);                      \
         i = k-i;                                                \
         s = sinwrk[i];                                          \
         c = coswrk[i];                                          \
         if (i>1)                                                \
            {                                                    \
             for (j=k-i+2 ; (1<<j)&t_lam ; j++);                 \
             j         = k - j;                                  \
             sinwrk[i] = halsec[i] * (sinwrk[i-1] + sinwrk[j]);  \
             coswrk[i] = halsec[i] * (coswrk[i-1] + coswrk[j]);  \
            }                                                    \
     }
#endif


#define TRIG_TAB_SIZE 22

#ifndef ROCKBOX
static long long halsec[TRIG_TAB_SIZE]= {1,2,3};
#endif

#define FFTmult(x,y) mult(x,y)




#include "d_imayer_tables.h"

#define SQRT2    ftofix(1.414213562373095048801688724209698)


static void imayer_fht(t_fixed *fz, int n)
{
 int  k,k1,k2,k3,k4,kx;
 t_fixed *fi,*fn,*gi;
 TRIG_VARS;

 for (k1=1,k2=0;k1<n;k1++)
    {
     t_fixed aa;
     for (k=n>>1; (!((k2^=k)&k)); k>>=1);
     if (k1>k2)
	{
	     aa=fz[k1];fz[k1]=fz[k2];fz[k2]=aa;
	}
    }
 for ( k=0 ; (1<<k)<n ; k++ );
 k  &= 1;
 if (k==0)
    {
	 for (fi=fz,fn=fz+n;fi<fn;fi+=4)
	    {
	     t_fixed f0,f1,f2,f3;
	     f1     = fi[0 ]-fi[1 ];
	     f0     = fi[0 ]+fi[1 ];
	     f3     = fi[2 ]-fi[3 ];
	     f2     = fi[2 ]+fi[3 ];
	     fi[2 ] = (f0-f2);	
	     fi[0 ] = (f0+f2);
	     fi[3 ] = (f1-f3);	
	     fi[1 ] = (f1+f3);
	    }
    }
 else
    {
	 for (fi=fz,fn=fz+n,gi=fi+1;fi<fn;fi+=8,gi+=8)
	    {
	     t_fixed bs1,bc1,bs2,bc2,bs3,bc3,bs4,bc4,
	     	bg0,bf0,bf1,bg1,bf2,bg2,bf3,bg3;
	     bc1     = fi[0 ] - gi[0 ];
	     bs1     = fi[0 ] + gi[0 ];
	     bc2     = fi[2 ] - gi[2 ];
	     bs2     = fi[2 ] + gi[2 ];
	     bc3     = fi[4 ] - gi[4 ];
	     bs3     = fi[4 ] + gi[4 ];
	     bc4     = fi[6 ] - gi[6 ];
	     bs4     = fi[6 ] + gi[6 ];
	     bf1     = (bs1 - bs2);	
	     bf0     = (bs1 + bs2);
	     bg1     = (bc1 - bc2);	
	     bg0     = (bc1 + bc2);
	     bf3     = (bs3 - bs4);	
	     bf2     = (bs3 + bs4);
	     bg3     = FFTmult(SQRT2,bc4);		
	     bg2     = FFTmult(SQRT2,bc3);
	     fi[4 ] = bf0 - bf2;
	     fi[0 ] = bf0 + bf2;
	     fi[6 ] = bf1 - bf3;
	     fi[2 ] = bf1 + bf3;
	     gi[4 ] = bg0 - bg2;
	     gi[0 ] = bg0 + bg2;
	     gi[6 ] = bg1 - bg3;
	     gi[2 ] = bg1 + bg3;
	    }
    }
 if (n<16) return;

 do
    {
     t_fixed s1,c1;
     int ii;
     k  += 2;
     k1  = 1  << k;
     k2  = k1 << 1;
     k4  = k2 << 1;
     k3  = k2 + k1;
     kx  = k1 >> 1;
	 fi  = fz;
	 gi  = fi + kx;
	 fn  = fz + n;
	 do
	    {
	     t_fixed g0,f0,f1,g1,f2,g2,f3,g3;
	     f1      = fi[0 ] - fi[k1];
	     f0      = fi[0 ] + fi[k1];
	     f3      = fi[k2] - fi[k3];
	     f2      = fi[k2] + fi[k3];
	     fi[k2]  = f0	  - f2;
	     fi[0 ]  = f0	  + f2;
	     fi[k3]  = f1	  - f3;
	     fi[k1]  = f1	  + f3;
	     g1      = gi[0 ] - gi[k1];
	     g0      = gi[0 ] + gi[k1];
	     g3      = FFTmult(SQRT2, gi[k3]);
	     g2      = FFTmult(SQRT2, gi[k2]);
	     gi[k2]  = g0	  - g2;
	     gi[0 ]  = g0	  + g2;
	     gi[k3]  = g1	  - g3;
	     gi[k1]  = g1	  + g3;
	     gi     += k4;
	     fi     += k4;
	    } while (fi<fn);
     TRIG_INIT(k,c1,s1);
     for (ii=1;ii<kx;ii++)
        {
	 t_fixed c2,s2;
         TRIG_NEXT(k,c1,s1);
         c2 = FFTmult(c1,c1) - FFTmult(s1,s1);
         s2 = 2*FFTmult(c1,s1);
	     fn = fz + n;
	     fi = fz +ii;
	     gi = fz +k1-ii;
	     do
		{
		 t_fixed a,b,g0,f0,f1,g1,f2,g2,f3,g3;
		 b       = FFTmult(s2,fi[k1]) - FFTmult(c2,gi[k1]);
		 a       = FFTmult(c2,fi[k1]) + FFTmult(s2,gi[k1]);
		 f1      = fi[0 ]    - a;
		 f0      = fi[0 ]    + a;
		 g1      = gi[0 ]    - b;
		 g0      = gi[0 ]    + b;
		 b       = FFTmult(s2,fi[k3]) - FFTmult(c2,gi[k3]);
		 a       = FFTmult(c2,fi[k3]) + FFTmult(s2,gi[k3]);
		 f3      = fi[k2]    - a;
		 f2      = fi[k2]    + a;
		 g3      = gi[k2]    - b;
		 g2      = gi[k2]    + b;
		 b       = FFTmult(s1,f2)     - FFTmult(c1,g3);
		 a       = FFTmult(c1,f2)     + FFTmult(s1,g3);
		 fi[k2]  = f0        - a;
		 fi[0 ]  = f0        + a;
		 gi[k3]  = g1        - b;
		 gi[k1]  = g1        + b;
		 b       = FFTmult(c1,g2)     - FFTmult(s1,f3);
		 a       = FFTmult(s1,g2)     + FFTmult(c1,f3);
		 gi[k2]  = g0        - a;
		 gi[0 ]  = g0        + a;
		 fi[k3]  = f1        - b;
		 fi[k1]  = f1        + b;
		 gi     += k4;
		 fi     += k4;
		} while (fi<fn);
        }
     TRIG_RESET(k,c1,s1);
    } while (k4<n);
}


void imayer_fft(int n, t_fixed *real, t_fixed *imag)
{
 t_fixed a,b,c,d;
 t_fixed q,r,s,t;
 int i,j,k;
 for (i=1,j=n-1,k=n/2;i<k;i++,j--) {
  a = real[i]; b = real[j];  q=a+b; r=a-b;
  c = imag[i]; d = imag[j];  s=c+d; t=c-d;
  real[i] = (q+t)>>1; real[j] = (q-t)>>1;
  imag[i] = (s-r)>>1; imag[j] = (s+r)>>1;
 }
 imayer_fht(real,n);
 imayer_fht(imag,n);
}

void imayer_ifft(int n, t_fixed *real, t_fixed *imag)
{
 t_fixed a,b,c,d;
 t_fixed q,r,s,t;
 int i,j,k;
 imayer_fht(real,n);
 imayer_fht(imag,n);
 for (i=1,j=n-1,k=n/2;i<k;i++,j--) {
  a = real[i]; b = real[j];  q=a+b; r=a-b;
  c = imag[i]; d = imag[j];  s=c+d; t=c-d;
  imag[i] = (s+r)>>1;  imag[j] = (s-r)>>1;
  real[i] = (q-t)>>1;  real[j] = (q+t)>>1;
 }
}

void imayer_realfft(int n, t_fixed *real)
{
#ifdef ROCKBOX
 t_fixed a,b;
#else
 t_fixed a,b,c,d;
#endif
 int i,j,k;
 imayer_fht(real,n);
 for (i=1,j=n-1,k=n/2;i<k;i++,j--) {
  a = real[i];
  b = real[j];
  real[j] = (a-b)>>1;
  real[i] = (a+b)>>1;
 }
}

void imayer_realifft(int n, t_fixed *real)
{
#ifdef ROCKBOX
 t_fixed a,b;
#else
 t_fixed a,b,c,d;
#endif
 int i,j,k;
 for (i=1,j=n-1,k=n/2;i<k;i++,j--) {
  a = real[i];
  b = real[j];
  real[j] = (a-b);
  real[i] = (a+b);
 }
 imayer_fht(real,n);
}



#ifdef TEST

static double dfcostab[TRIG_TAB_SIZE]=
    {
     .00000000000000000000000000000000000000000000000000,
     .70710678118654752440084436210484903928483593768847,
     .92387953251128675612818318939678828682241662586364,
     .98078528040323044912618223613423903697393373089333,
     .99518472667219688624483695310947992157547486872985,
     .99879545620517239271477160475910069444320361470461,
     .99969881869620422011576564966617219685006108125772,
     .99992470183914454092164649119638322435060646880221,
     .99998117528260114265699043772856771617391725094433,
     .99999529380957617151158012570011989955298763362218,
     .99999882345170190992902571017152601904826792288976,
     .99999970586288221916022821773876567711626389934930,
     .99999992646571785114473148070738785694820115568892,
     .99999998161642929380834691540290971450507605124278,
     .99999999540410731289097193313960614895889430318945,
     .99999999885102682756267330779455410840053741619428,
     .99999999971275670684941397221864177608908945791828,
     .99999999992818917670977509588385049026048033310951
    };
static double dfsintab[TRIG_TAB_SIZE]=
    {
     1.0000000000000000000000000000000000000000000000000,
     .70710678118654752440084436210484903928483593768846,
     .38268343236508977172845998403039886676134456248561,
     .19509032201612826784828486847702224092769161775195,
     .09801714032956060199419556388864184586113667316749,
     .04906767432741801425495497694268265831474536302574,
     .02454122852291228803173452945928292506546611923944,
     .01227153828571992607940826195100321214037231959176,
     .00613588464915447535964023459037258091705788631738,
     .00306795676296597627014536549091984251894461021344,
     .00153398018628476561230369715026407907995486457522,
     .00076699031874270452693856835794857664314091945205,
     .00038349518757139558907246168118138126339502603495,
     .00019174759731070330743990956198900093346887403385,
     .00009587379909597734587051721097647635118706561284,
     .00004793689960306688454900399049465887274686668768,
     .00002396844980841821872918657716502182009476147489,
     .00001198422490506970642152156159698898480473197753
    };

static double dhalsec[TRIG_TAB_SIZE]=
    {
     0,
     0,
     .54119610014619698439972320536638942006107206337801,
     .50979557910415916894193980398784391368261849190893,
     .50241928618815570551167011928012092247859337193963,
     .50060299823519630134550410676638239611758632599591,
     .50015063602065098821477101271097658495974913010340,
     .50003765191554772296778139077905492847503165398345,
     .50000941253588775676512870469186533538523133757983,
     .50000235310628608051401267171204408939326297376426,
     .50000058827484117879868526730916804925780637276181,
     .50000014706860214875463798283871198206179118093251,
     .50000003676714377807315864400643020315103490883972,
     .50000000919178552207366560348853455333939112569380,
     .50000000229794635411562887767906868558991922348920,
     .50000000057448658687873302235147272458812263401372,
     .50000000014362164661654736863252589967935073278768,
     .50000000003590541164769084922906986545517021050714
    };


#include <stdio.h>


int writetables()
{
  int i;

  printf("/* Tables for fixed point lookup with %d bit precision*/\n\n",fix1);

  printf("static int fsintab[TRIG_TAB_SIZE]= {\n");

  for (i=0;i<TRIG_TAB_SIZE-1;i++)
    printf("%ld, \n",ftofix(dfsintab[i]));
  printf("%ld }; \n",ftofix(dfsintab[i]));


  printf("\n\nstatic int fcostab[TRIG_TAB_SIZE]= {\n");

  for (i=0;i<TRIG_TAB_SIZE-1;i++)
    printf("%ld, \n",ftofix(dfcostab[i]));
  printf("%ld }; \n",ftofix(dfcostab[i]));

}


#define OUTPUT \
  fprintf(stderr,"Integer - Float\n");\
  for (i=0;i<NP;i++)\
    fprintf(stderr,"%f %f --> %f %f\n",fixtof(r[i]),fixtof(im[i]),\
      fr[i],fim[i]);\
  fprintf(stderr,"\n\n");



int main()
{
  int i;
  t_fixed r[256];
  t_fixed im[256];
  float fr[256];
  float fim[256];


#if 1
     writetables();
     exit(0);
#endif


#if 0
  t_fixed c1,s1,c2,s2,c3,s3;
  int k;
  int i;

  TRIG_VARS;
 
  for (k=2;k<10;k+=2) {
    TRIG_INIT(k,c1,s1);
    for (i=0;i<8;i++) {
      TRIG_NEXT(k,c1,s1);
      TRIG_23(k,c1,s1,c2,s2,c3,s3);
      printf("TRIG value k=%d,%d val1 = %f %f\n",k,i,fixtof(c1),fixtof(s1));
    }
  }
#endif



#if 1

  #define NP 16

  for (i=0;i<NP;i++) { 
    fr[i] = 0.;
    r[i] = 0.;
    fim[i] = 0.;
    im[i] = 0;
  }

#if 0
  for (i=0;i<NP;i++) { 
    if (i&1) {
      fr[i] = 0.1*i;
      r[i] = ftofix(0.1*i);
    }
    else {
	 fr[i] = 0.;
	 r[i] = 0.;
    }
  }
#endif
#if 0
  for (i=0;i<NP;i++) { 
       fr[i] = 0.1;
       r[i] = ftofix(0.1);
  }
#endif

  r[1] = ftofix(0.1);
  fr[1] = 0.1;



  /* TEST RUN */

  OUTPUT

  imayer_fft(NP,r,im);
  mayer_fft(NP,fr,fim);
//  imayer_fht(r,NP);
//  mayer_fht(fr,NP);

#if 1
  for (i=0;i<NP;i++) { 
       r[i] = mult(ftofix(0.01),r[i]);
       fr[i] = 0.01*fr[i];
  }
#endif

  OUTPUT


  imayer_fft(NP,r,im);
  mayer_fft(NP,fr,fim);

  OUTPUT


#endif


}

#endif

