/*
 * $ProjectName$
 * $ProjectRevision$
 * -----------------------------------------------------------
 * $Id$multip
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
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.vorbis;

import java.io.IOException;
import java.util.*;

import de.jarnbjo.util.io.BitInputStream;


class Floor1 extends Floor implements Cloneable {

   private int[] partitionClassList;
   private int maximumClass, multiplier, rangeBits;
   private int[] classDimensions;
   private int[] classSubclasses;
   private int[] classMasterbooks;
   private int[][] subclassBooks;
   private int[] xList;
   private int[] yList;
   private int[] lowNeighbours, highNeighbours;
   //private boolean[] step2Flags;

   private static final int[] RANGES = {256, 128, 86, 64};

   private Floor1() {
   }

   protected Floor1(BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {

      maximumClass=-1;
      int partitions=source.getInt(5);
      partitionClassList=new int[partitions];

      for(int i=0; i<partitionClassList.length; i++) {
         partitionClassList[i]=source.getInt(4);
         if(partitionClassList[i]>maximumClass) {
            maximumClass=partitionClassList[i];
         }
      }


      classDimensions=new int[maximumClass+1];
      classSubclasses=new int[maximumClass+1];
      classMasterbooks=new int[maximumClass+1];
      subclassBooks=new int[maximumClass+1][];

      int xListLength=2;

      for(int i=0; i<=maximumClass; i++) {
         classDimensions[i]=source.getInt(3)+1;
         xListLength+=classDimensions[i];
         classSubclasses[i]=source.getInt(2);

         if(classDimensions[i] > header.getCodeBooks().length ||
            classSubclasses[i] > header.getCodeBooks().length) {
            throw new VorbisFormatException("There is a class dimension or class subclasses entry higher than the number of codebooks in the setup header.");
         }
         if(classSubclasses[i]!=0) {
            classMasterbooks[i]=source.getInt(8);
         }
         subclassBooks[i]=new int[1<<classSubclasses[i]];
         for(int j=0; j<subclassBooks[i].length; j++) {
            subclassBooks[i][j]=source.getInt(8)-1;
         }
      }

      multiplier=source.getInt(2)+1;
      rangeBits=source.getInt(4);

      //System.out.println("multiplier: "+multiplier);
      //System.out.println("rangeBits: "+rangeBits);

      //System.out.println("xListLength: "+xListLength);

      int floorValues=0;

      ArrayList alXList=new ArrayList();

      alXList.add(new Integer(0));
      alXList.add(new Integer(1<<rangeBits));

      //System.out.println("partitions: "+partitions);
      //System.out.println("classDimensions.length: "+classDimensions.length);

      for(int i=0; i<partitions; i++) {
         for(int j=0; j<classDimensions[partitionClassList[i]]; j++) {
            alXList.add(new Integer(source.getInt(rangeBits)));
         }
      }

      xList=new int[alXList.size()];
      lowNeighbours=new int[xList.length];
      highNeighbours=new int[xList.length];

      Iterator iter=alXList.iterator();
      for(int i=0; i<xList.length; i++) {
         xList[i]=((Integer)iter.next()).intValue();
      }

      for(int i=0; i<xList.length; i++) {
         lowNeighbours[i]=Util.lowNeighbour(xList, i);
         highNeighbours[i]=Util.highNeighbour(xList, i);
      }
   }

   protected int getType() {
      return 1;
   }

   protected Floor decodeFloor(VorbisStream vorbis, BitInputStream source) throws VorbisFormatException, IOException {

   //System.out.println("decodeFloor");
      if(!source.getBit()) {
         //System.out.println("null");
         return null;
      }

      Floor1 clone=(Floor1)clone();

      clone.yList=new int[xList.length];

      int range=RANGES[multiplier-1];

      clone.yList[0]=source.getInt(Util.ilog(range-1));
      clone.yList[1]=source.getInt(Util.ilog(range-1));

      int offset=2;

      for(int i=0; i<partitionClassList.length; i++) {
         int cls=partitionClassList[i];
         int cdim=classDimensions[cls];
         int cbits=classSubclasses[cls];
         int csub=(1<<cbits)-1;
         int cval=0;
         if(cbits>0) {
            cval=source.getInt(vorbis.getSetupHeader().getCodeBooks()[classMasterbooks[cls]].getHuffmanRoot());
            //cval=vorbis.getSetupHeader().getCodeBooks()[classMasterbooks[cls]].readInt(source);
            //System.out.println("cval: "+cval);
         }
         //System.out.println("0: "+cls+" "+cdim+" "+cbits+" "+csub+" "+cval);
         for(int j=0; j<cdim; j++) {
            //System.out.println("a: "+cls+" "+cval+" "+csub);
            int book=subclassBooks[cls][cval&csub];
            cval>>>=cbits;
            if(book>=0) {
               clone.yList[j+offset]=source.getInt(vorbis.getSetupHeader().getCodeBooks()[book].getHuffmanRoot());
               //clone.yList[j+offset]=vorbis.getSetupHeader().getCodeBooks()[book].readInt(source);
               //System.out.println("b: "+(j+offset)+" "+book+" "+clone.yList[j+offset]);
               //System.out.println("");
            }
            else {
               clone.yList[j+offset]=0;
            }
         }
         offset+=cdim;
      }

      //System.out.println("");
      //for(int i=0; i<clone.xList.length; i++) {
      //   System.out.println(i+" = "+clone.xList[i]);
      //}

      //System.out.println("");
      //for(int i=0; i<clone.yList.length; i++) {
      //   System.out.println(i+" = "+clone.yList[i]);
      //}

      //System.out.println("offset: "+offset);
      //System.out.println("yList.length: "+clone.yList.length);

      //System.exit(0);

      return clone;
   }

   protected void computeFloor(final float[] vector) {

      int n=vector.length;
      final int values=xList.length;
      final boolean[] step2Flags=new boolean[values];

      final int range=RANGES[multiplier-1];

      for(int i=2; i<values; i++) {
         final int lowNeighbourOffset=lowNeighbours[i];//Util.lowNeighbour(xList, i);
         final int highNeighbourOffset=highNeighbours[i];//Util.highNeighbour(xList, i);
         final int predicted=Util.renderPoint(
            xList[lowNeighbourOffset], xList[highNeighbourOffset],
            yList[lowNeighbourOffset], yList[highNeighbourOffset],
            xList[i]);
         final int val=yList[i];
         final int highRoom=range-predicted;
         final int lowRoom=predicted;
         final int room=highRoom<lowRoom?highRoom*2:lowRoom*2;
         if(val!=0) {
            step2Flags[lowNeighbourOffset]=true;
            step2Flags[highNeighbourOffset]=true;
            step2Flags[i]=true;
            if(val>=room) {
               yList[i]=highRoom>lowRoom?
                  val-lowRoom+predicted:
                  -val+highRoom+predicted-1;
            }
            else {
               yList[i]=(val&1)==1?
                  predicted-((val+1)>>1):
                  predicted+(val>>1);
            }
         }
         else {
            step2Flags[i]=false;
            yList[i]=predicted;
         }
      }

      final int[] xList2=new int[values];

      System.arraycopy(xList, 0, xList2, 0, values);
      sort(xList2, yList, step2Flags);

      int hx=0, hy=0, lx=0, ly=yList[0]*multiplier;

      float[] vector2=new float[vector.length];
      float[] vector3=new float[vector.length];
      Arrays.fill(vector2, 1.0f);
      System.arraycopy(vector, 0, vector3, 0, vector.length);

      for(int i=1; i<values; i++) {
         if(step2Flags[i]) {
            hy=yList[i]*multiplier;
            hx=xList2[i];
            Util.renderLine(lx, ly, hx, hy, vector);
            Util.renderLine(lx, ly, hx, hy, vector2);
            lx=hx;
            ly=hy;
         }
      }

      final float r=DB_STATIC_TABLE[hy];
      for(; hx<n/2; vector[hx++]=r);
   }

   public Object clone() {
      Floor1 clone=new Floor1();
      clone.classDimensions=classDimensions;
      clone.classMasterbooks=classMasterbooks;
      clone.classSubclasses=classSubclasses;
      clone.maximumClass=maximumClass;
      clone.multiplier=multiplier;
      clone.partitionClassList=partitionClassList;
      clone.rangeBits=rangeBits;
      clone.subclassBooks=subclassBooks;
      clone.xList=xList;
      clone.yList=yList;
      clone.lowNeighbours=lowNeighbours;
      clone.highNeighbours=highNeighbours;
      return clone;
   }

   private final static void sort(int x[], int y[], boolean b[]) {
      int off=0;
      int len=x.length;
      int lim=len+off;
      int itmp;
      boolean btmp;
	// Insertion sort on smallest arrays
      for (int i=off; i<lim; i++) {
         for (int j=i; j>off && x[j-1]>x[j]; j--) {
            itmp=x[j];
            x[j]=x[j-1];
            x[j-1]=itmp;
            itmp=y[j];
            y[j]=y[j-1];
            y[j-1]=itmp;
            btmp=b[j];
            b[j]=b[j-1];
            b[j-1]=btmp;
            //swap(x, j, j-1);
            //swap(y, j, j-1);
            //swap(b, j, j-1);
         }
      }
	}

   private final static void swap(int x[], int a, int b) {
	   int t = x[a];
	   x[a] = x[b];
	   x[b] = t;
   }

   private final static void swap(boolean x[], int a, int b) {
	   boolean t = x[a];
	   x[a] = x[b];
	   x[b] = t;
   }
}