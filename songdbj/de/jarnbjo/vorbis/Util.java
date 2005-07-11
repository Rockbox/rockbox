/*
 * $ProjectName$
 * $ProjectRevision$
 * -----------------------------------------------------------
 * $Id$
 * -----------------------------------------------------------
 *
 * $Author$
 *
 * Description:
 *
 * Copyright 2002-2003 Tor-Einar Jarnbjo
 * -----------------------------------------------------------
 *
 * Change History
 * -----------------------------------------------------------
 * $Log$
 * Revision 1.1  2005/07/11 15:42:36  hcl
 * Songdb java version, source. only 1.5 compatible
 *
 * Revision 1.1.1.1  2004/04/04 22:09:12  shred
 * First Import
 *
 * Revision 1.3  2003/04/10 19:49:04  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.vorbis;

final public class Util {

   public static final int ilog(int x) {
      int res=0;
      for(; x>0; x>>=1, res++);
      return res;
   }

   public static final float float32unpack(int x) {
      float mantissa=x&0x1fffff;
      float e=(x&0x7fe00000)>>21;
      if((x&0x80000000)!=0) {
         mantissa=-mantissa;
      }
      return mantissa*(float)Math.pow(2.0, e-788.0);
   }

   public static final int lookup1Values(int a, int b) {
      int res=(int)Math.pow(Math.E, Math.log(a)/b);
      return intPow(res+1, b)<=a?res+1:res;
   }

   public static final int intPow(int base, int e) {
      int res=1;
      for(; e>0; e--, res*=base);
      return res;
   }

   public static final boolean isBitSet(int value, int bit) {
      return (value&(1<<bit))!=0;
   }

   public static final int icount(int value) {
      int res=0;
      while(value>0) {
         res+=value&1;
         value>>=1;
      }
      return res;
   }

   public static final int lowNeighbour(int[] v, int x) {
      int max=-1, n=0;
      for(int i=0; i<v.length && i<x; i++) {
         if(v[i]>max && v[i]<v[x]) {
            max=v[i];
            n=i;
         }
      }
      return n;
   }

   public static final int highNeighbour(int[] v, int x) {
      int min=Integer.MAX_VALUE, n=0;
      for(int i=0; i<v.length && i<x; i++) {
         if(v[i]<min && v[i]>v[x]) {
            min=v[i];
            n=i;
         }
      }
      return n;
   }

   public static final int renderPoint(int x0, int x1, int y0, int y1, int x) {
      int dy=y1-y0;
      int ady=dy<0?-dy:dy;
      int off=(ady*(x-x0))/(x1-x0);
      return dy<0?y0-off:y0+off;
   }

   public static final void renderLine(final int x0, final int y0, final int x1, final int y1, final float[] v) {
      final int dy=y1-y0;
      final int adx=x1-x0;
      final int base=dy/adx;
      final int sy=dy<0?base-1:base+1;
      int x=x0;
      int y=y0;
      int err=0;
      final int ady=(dy<0?-dy:dy)-(base>0?base*adx:-base*adx);

      v[x]*=Floor.DB_STATIC_TABLE[y];
      for(x=x0+1; x<x1; x++) {
         err+=ady;
         if(err>=adx) {
            err-=adx;
            v[x]*=Floor.DB_STATIC_TABLE[y+=sy];
         }
         else {
            v[x]*=Floor.DB_STATIC_TABLE[y+=base];
         }
      }
   }
}