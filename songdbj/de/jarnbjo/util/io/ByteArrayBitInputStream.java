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
 * Revision 1.3  2003/04/10 19:48:31  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/16 01:11:39  jarnbjo
 * no message
 *
 * Revision 1.1  2003/03/03 21:02:20  jarnbjo
 * no message
 *
 */

package de.jarnbjo.util.io;

import java.io.IOException;

/**
 *  Implementation of the <code>BitInputStream</code> interface,
 *  using a byte array as data source.
*/

public class ByteArrayBitInputStream implements BitInputStream {

   private byte[] source;
   private byte currentByte;

   private int endian;

   private int byteIndex=0;
   private int bitIndex=0;

   public ByteArrayBitInputStream(byte[] source) {
      this(source, LITTLE_ENDIAN);
   }

   public ByteArrayBitInputStream(byte[] source, int endian) {
      this.endian=endian;
      this.source=source;
      currentByte=source[0];
      bitIndex=(endian==LITTLE_ENDIAN)?0:7;
   }

   public boolean getBit() throws IOException {
      if(endian==LITTLE_ENDIAN) {
         if(bitIndex>7) {
            bitIndex=0;
            currentByte=source[++byteIndex];
         }
         return (currentByte&(1<<(bitIndex++)))!=0;
      }
      else {
         if(bitIndex<0) {
            bitIndex=7;
            currentByte=source[++byteIndex];
         }
         return (currentByte&(1<<(bitIndex--)))!=0;
      }
   }

   public int getInt(int bits) throws IOException {
      if(bits>32) {
         throw new IllegalArgumentException("Argument \"bits\" must be <= 32");
      }
      int res=0;
      if(endian==LITTLE_ENDIAN) {
         for(int i=0; i<bits; i++) {
            if(getBit()) {
               res|=(1<<i);
            }
         }
      }
      else {
         if(bitIndex<0) {
            bitIndex=7;
            currentByte=source[++byteIndex];
         }
         if(bits<=bitIndex+1) {
            int ci=((int)currentByte)&0xff;
            int offset=1+bitIndex-bits;
            int mask=((1<<bits)-1)<<offset;
            res=(ci&mask)>>offset;
            bitIndex-=bits;
         }
         else {
            res=(((int)currentByte)&0xff&((1<<(bitIndex+1))-1))<<(bits-bitIndex-1);
            bits-=bitIndex+1;
            currentByte=source[++byteIndex];
            while(bits>=8) {
               bits-=8;
               res|=(((int)source[byteIndex])&0xff)<<bits;
               currentByte=source[++byteIndex];
            }
            if(bits>0) {
               int ci=((int)source[byteIndex])&0xff;
               res|=(ci>>(8-bits))&((1<<bits)-1);
               bitIndex=7-bits;
            }
            else {
               currentByte=source[--byteIndex];
               bitIndex=-1;
            }
         }
      }

      return res;
   }

   public int getSignedInt(int bits) throws IOException {
      int raw=getInt(bits);
      if(raw>=1<<(bits-1)) {
         raw-=1<<bits;
      }
      return raw;
   }

   public int getInt(HuffmanNode root) throws IOException {
      while(root.value==null) {
         if(bitIndex>7) {
            bitIndex=0;
            currentByte=source[++byteIndex];
         }
         root=(currentByte&(1<<(bitIndex++)))!=0?root.o1:root.o0;
      }
      return root.value.intValue();
   }

   public long getLong(int bits) throws IOException {
      if(bits>64) {
         throw new IllegalArgumentException("Argument \"bits\" must be <= 64");
      }
      long res=0;
      if(endian==LITTLE_ENDIAN) {
         for(int i=0; i<bits; i++) {
            if(getBit()) {
               res|=(1L<<i);
            }
         }
      }
      else {
         for(int i=bits-1; i>=0; i--) {
            if(getBit()) {
               res|=(1L<<i);
            }
         }
      }
      return res;
   }

	/**
	 *  <p>reads an integer encoded as "signed rice" as described in
	 *  the FLAC audio format specification</p>
	 *	
	 *  <p><b>not supported for little endian</b></p>
	 *
	 *  @param order 
	 *  @return the decoded integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */
    
   public int readSignedRice(int order) throws IOException {

      int msbs=-1, lsbs=0, res=0;

      if(endian==LITTLE_ENDIAN) {
         // little endian
         throw new UnsupportedOperationException("ByteArrayBitInputStream.readSignedRice() is only supported in big endian mode");
      }
      else {
         // big endian

         byte cb=source[byteIndex];
         do {
            msbs++;
            if(bitIndex<0) {
               bitIndex=7;
               byteIndex++;
               cb=source[byteIndex];
            }
         } while((cb&(1<<bitIndex--))==0);

         int bits=order;

         if(bitIndex<0) {
            bitIndex=7;
            byteIndex++;
         }
         if(bits<=bitIndex+1) {
            int ci=((int)source[byteIndex])&0xff;
            int offset=1+bitIndex-bits;
            int mask=((1<<bits)-1)<<offset;
            lsbs=(ci&mask)>>offset;
            bitIndex-=bits;
         }
         else {
            lsbs=(((int)source[byteIndex])&0xff&((1<<(bitIndex+1))-1))<<(bits-bitIndex-1);
            bits-=bitIndex+1;
            byteIndex++;
            while(bits>=8) {
               bits-=8;
               lsbs|=(((int)source[byteIndex])&0xff)<<bits;
               byteIndex++;
            }
            if(bits>0) {
               int ci=((int)source[byteIndex])&0xff;
               lsbs|=(ci>>(8-bits))&((1<<bits)-1);
               bitIndex=7-bits;
            }
            else {
               byteIndex--;
               bitIndex=-1;
            }
         }

         res=(msbs<<order)|lsbs;
      }

      return (res&1)==1?-(res>>1)-1:(res>>1);
   }

	/**
	 *  <p>fills the array from <code>offset</code> with <code>len</code> 
	 *  integers encoded as "signed rice" as described in
	 *  the FLAC audio format specification</p>
	 *	
	 *  <p><b>not supported for little endian</b></p>
	 *
	 *  @param order 
	 *  @param buffer
	 *  @param offset
	 *  @param len 
	 *  @return the decoded integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public void readSignedRice(int order, int[] buffer, int off, int len) throws IOException {

      if(endian==LITTLE_ENDIAN) {
         // little endian
         throw new UnsupportedOperationException("ByteArrayBitInputStream.readSignedRice() is only supported in big endian mode");
      }
      else {
         // big endian
         for(int i=off; i<off+len; i++) {

            int msbs=-1, lsbs=0;

            byte cb=source[byteIndex];
            do {
               msbs++;
               if(bitIndex<0) {
                  bitIndex=7;
                  byteIndex++;
                  cb=source[byteIndex];
               }
            } while((cb&(1<<bitIndex--))==0);

            int bits=order;

            if(bitIndex<0) {
               bitIndex=7;
               byteIndex++;
            }
            if(bits<=bitIndex+1) {
               int ci=((int)source[byteIndex])&0xff;
               int offset=1+bitIndex-bits;
               int mask=((1<<bits)-1)<<offset;
               lsbs=(ci&mask)>>offset;
               bitIndex-=bits;
            }
            else {
               lsbs=(((int)source[byteIndex])&0xff&((1<<(bitIndex+1))-1))<<(bits-bitIndex-1);
               bits-=bitIndex+1;
               byteIndex++;
               while(bits>=8) {
                  bits-=8;
                  lsbs|=(((int)source[byteIndex])&0xff)<<bits;
                  byteIndex++;
               }
               if(bits>0) {
                  int ci=((int)source[byteIndex])&0xff;
                  lsbs|=(ci>>(8-bits))&((1<<bits)-1);
                  bitIndex=7-bits;
               }
               else {
                  byteIndex--;
                  bitIndex=-1;
               }
            }

            int res=(msbs<<order)|lsbs;
            buffer[i]=(res&1)==1?-(res>>1)-1:(res>>1);
         }
      }
   }

   public void align() {
      if(endian==BIG_ENDIAN && bitIndex>=0) {
         bitIndex=7;
         byteIndex++;
      }
      else if(endian==LITTLE_ENDIAN && bitIndex<=7) {
         bitIndex=0;
         byteIndex++;
      }
   }

   public void setEndian(int endian) {
      if(this.endian==BIG_ENDIAN && endian==LITTLE_ENDIAN) {
         bitIndex=0;
         byteIndex++;
      }
      else if(this.endian==LITTLE_ENDIAN && endian==BIG_ENDIAN) {
         bitIndex=7;
         byteIndex++;
      }
      this.endian=endian;
   }

	/**
	 *  @return the byte array used as a source for this instance
	 */

   public byte[] getSource() {
      return source;
   }
}