#ifndef ABS
#define ABS(a) (((a) < 0) ? -(a) : (a))
#endif

#define MINIMUM 0.000000000001   /* e-12 */

/* -----------------------------------------------------------------------
   mySqrt uses Heron's algorithm, which is the Newtone-Raphson algorhitm
   in it's private case for sqrt.
   Thanks BlueChip for his intro text and Dave Straayer for the actual name.
   ----------------------------------------------------------------------- */
double sqrt(double square)
{
    int k = 0;
    double temp = 0;
    double root= ABS(square+1)/2;

    while( ABS(root - temp) > MINIMUM ){
        temp = root;
        root = (square/temp + temp)/2;
        k++;
        if (k>10000) return 0;
    }

    return root;
}

/*Uses the sequence sum(x^k/k!) that tends to exp(x)*/
double exp (double x) {
    unsigned int k=0;
    double res=0, xPow=1,fact=1,toAdd;

    do {
        toAdd = xPow/fact;
        res += toAdd;
        xPow *= x; //xPow = x^k
        k++;
        fact*=k; //fact = k!
    } while (ABS(toAdd) > MINIMUM && xPow<1e302);
    return res;
}

