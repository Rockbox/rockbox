/* -*-mode:java; c-basic-offset:2; -*- */
/* JOrbis
 * Copyright (C) 2000 ymnk, JCraft,Inc.
 *  
 * Written by: 2000 ymnk<ymnk@jcraft.com>
 *   
 * Many thanks to 
 *   Monty <monty@xiph.org> and 
 *   The XIPHOPHORUS Company http://www.xiph.org/ .
 * JOrbis has been based on their awesome works, Vorbis codec.
 *   
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
   
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

package com.jcraft.jogg;

public class Buffer{
  private static final int BUFFER_INCREMENT=256;

  private static final int[] mask={
    0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
    0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
    0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
    0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
    0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
    0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
    0x3fffffff,0x7fffffff,0xffffffff
  };

  int ptr=0;
  byte[] buffer=null;
  int endbit=0;
  int endbyte=0;
  int storage=0;

  public void writeinit(){
    buffer=new byte[BUFFER_INCREMENT]; 
    ptr=0;
    buffer[0]=(byte)'\0';
    storage=BUFFER_INCREMENT;
  }

  public void write(byte[] s){
    for(int i=0; i<s.length; i++){
      if(s[i]==0)break;
      write(s[i],8);
    }
  }

  public void read(byte[] s, int bytes){
    int i=0;
    while(bytes--!=0){
      s[i++]=(byte)(read(8));
    }
  }

  void reset(){
    ptr=0;
    buffer[0]=(byte)'\0';
    endbit=endbyte=0;
  }

  public void writeclear(){
    buffer=null;
  }

  public void readinit(byte[] buf, int bytes){
    readinit(buf, 0, bytes);
  }

  public void readinit(byte[] buf, int start, int bytes){
//System.err.println("readinit: start="+start+", bytes="+bytes);
//for(int i=0;i<bytes; i++){
//System.err.println(i+": "+Integer.toHexString(buf[i+start]));
//}
    ptr=start;
    buffer=buf;
    endbit=endbyte=0;
    storage=bytes;
  }

  public void write(int value, int bits){
//System.err.println("write: "+Integer.toHexString(value)+", bits="+bits+" ptr="+ptr+", storage="+storage+", endbyte="+endbyte);
    if(endbyte+4>=storage){
      byte[] foo=new byte[storage+BUFFER_INCREMENT];
      System.arraycopy(buffer, 0, foo, 0, storage);
      buffer=foo;
      storage+=BUFFER_INCREMENT;
    }

    value&=mask[bits];
    bits+=endbit;
    buffer[ptr]|=(byte)(value<<endbit);

    if(bits>=8){
      buffer[ptr+1]=(byte)(value>>>(8-endbit));
      if(bits>=16){
        buffer[ptr+2]=(byte)(value>>>(16-endbit));  
        if(bits>=24){
	  buffer[ptr+3]=(byte)(value>>>(24-endbit));  
          if(bits>=32){
	    if(endbit>0)
	      buffer[ptr+4]=(byte)(value>>>(32-endbit));
	    else
	      buffer[ptr+4]=0;
	  }
        }
      }
    }

    endbyte+=bits/8;
    ptr+=bits/8;
    endbit=bits&7;
  }

  public int look(int bits){
    int ret;
    int m=mask[bits];

    bits+=endbit;

//System.err.println("look ptr:"+ptr+", bits="+bits+", endbit="+endbit+", storage="+storage);

    if(endbyte+4>=storage){
      if(endbyte+(bits-1)/8>=storage)return(-1);
    }
  
    ret=((buffer[ptr])&0xff)>>>endbit;
//  ret=((byte)(buffer[ptr]))>>>endbit;
    if(bits>8){
    ret|=((buffer[ptr+1])&0xff)<<(8-endbit);
//      ret|=((byte)(buffer[ptr+1]))<<(8-endbit);
      if(bits>16){
      ret|=((buffer[ptr+2])&0xff)<<(16-endbit);
//        ret|=((byte)(buffer[ptr+2]))<<(16-endbit);
        if(bits>24){
	  ret|=((buffer[ptr+3])&0xff)<<(24-endbit);
//System.err.print("ret="+Integer.toHexString(ret)+", ((byte)(buffer[ptr+3]))="+Integer.toHexString(((buffer[ptr+3])&0xff)));
//	  ret|=((byte)(buffer[ptr+3]))<<(24-endbit);
//System.err.println(" ->ret="+Integer.toHexString(ret));
 	  if(bits>32 && endbit!=0){
	    ret|=((buffer[ptr+4])&0xff)<<(32-endbit);
//	    ret|=((byte)(buffer[ptr+4]))<<(32-endbit);
	  }
        }
      }
    }
    return(m&ret);
  }

  public int look1(){
    if(endbyte>=storage)return(-1);
    return((buffer[ptr]>>endbit)&1);
  }

  public void adv(int bits){
    bits+=endbit;
    ptr+=bits/8;
    endbyte+=bits/8;
    endbit=bits&7;
  }

  public void adv1(){
    ++endbit;
    if(endbit>7){
      endbit=0;
      ptr++;
      endbyte++;
    }
  }

  public int read(int bits){
//System.err.println(this+" read: bits="+bits+", storage="+storage+", endbyte="+endbyte);
//System.err.println(this+" read: bits="+bits+", storage="+storage+", endbyte="+endbyte+
//                        ", ptr="+ptr+", endbit="+endbit+", buf[ptr]="+buffer[ptr]);

    int ret;
    int m=mask[bits];

    bits+=endbit;

    if(endbyte+4>=storage){
      ret=-1;
      if(endbyte+(bits-1)/8>=storage){
        ptr+=bits/8;
        endbyte+=bits/8;
        endbit=bits&7;
        return(ret);
      }
    }

/*  
    ret=(byte)(buffer[ptr]>>>endbit);
    if(bits>8){
      ret|=(buffer[ptr+1]<<(8-endbit));  
      if(bits>16){
        ret|=(buffer[ptr+2]<<(16-endbit));  
        if(bits>24){
	  ret|=(buffer[ptr+3]<<(24-endbit));  
  	  if(bits>32 && endbit>0){
  	    ret|=(buffer[ptr+4]<<(32-endbit));
  	  }
        }
      }
    }
*/
    ret=((buffer[ptr])&0xff)>>>endbit;
    if(bits>8){
    ret|=((buffer[ptr+1])&0xff)<<(8-endbit);
//      ret|=((byte)(buffer[ptr+1]))<<(8-endbit);
      if(bits>16){
      ret|=((buffer[ptr+2])&0xff)<<(16-endbit);
//        ret|=((byte)(buffer[ptr+2]))<<(16-endbit);
        if(bits>24){
	  ret|=((buffer[ptr+3])&0xff)<<(24-endbit);
//	  ret|=((byte)(buffer[ptr+3]))<<(24-endbit);
 	  if(bits>32 && endbit!=0){
	    ret|=((buffer[ptr+4])&0xff)<<(32-endbit);
//	    ret|=((byte)(buffer[ptr+4]))<<(32-endbit);
	  }
        }
      }
    }

    ret&=m;

    ptr+=bits/8;
//    ptr=bits/8;
    endbyte+=bits/8;
//    endbyte=bits/8;
    endbit=bits&7;
    return(ret);
  }

  public int readB(int bits){
    //System.err.println(this+" read: bits="+bits+", storage="+storage+", endbyte="+endbyte+
    //                        ", ptr="+ptr+", endbit="+endbit+", buf[ptr]="+buffer[ptr]);
    int ret;
    int m=32-bits;

    bits+=endbit;

    if(endbyte+4>=storage){
      /* not the main path */
      ret=-1;
      if(endbyte*8+bits>storage*8) {
	ptr+=bits/8;
	endbyte+=bits/8;
	endbit=bits&7;
	return(ret);
      }
    }

    ret=(buffer[ptr]&0xff)<<(24+endbit);
    if(bits>8){
      ret|=(buffer[ptr+1]&0xff)<<(16+endbit);
      if(bits>16){
	ret|=(buffer[ptr+2]&0xff)<<(8+endbit);
	if(bits>24){
	  ret|=(buffer[ptr+3]&0xff)<<(endbit);
	  if(bits>32 && (endbit != 0))
	    ret|=(buffer[ptr+4]&0xff)>>(8-endbit);
	}
      }
    }
    ret=(ret>>>(m>>1))>>>((m+1)>>1);

    ptr+=bits/8;
    endbyte+=bits/8;
    endbit=bits&7;
    return(ret);
  }

  public int read1(){
    int ret;
    if(endbyte>=storage){
      ret=-1;
      endbit++;
      if(endbit>7){
        endbit=0;
        ptr++;
        endbyte++;
      }
      return(ret);
    }

    ret=(buffer[ptr]>>endbit)&1;

    endbit++;
    if(endbit>7){
      endbit=0;
      ptr++;
      endbyte++;
    }
    return(ret);
  }

  public int bytes(){
    return(endbyte+(endbit+7)/8);
  }

  public int bits(){
    return(endbyte*8+endbit);
  }

  public byte[] buffer(){
    return(buffer);
  }

  public static int ilog(int v){
    int ret=0;
    while(v>0){
      ret++;
      v>>>=1;
    }
    return(ret);
  }

  public static void report(String in){
    System.err.println(in);
    System.exit(1);
  }

  /*
  static void cliptest(int[] b, int vals, int bits, int[] comp, int compsize){
    int bytes;
    byte[] buffer;

    o.reset();
    for(int i=0;i<vals;i++){
      o.write(b[i],((bits!=0)?bits:ilog(b[i])));
    }
    buffer=o.buffer();
    bytes=o.bytes();
System.err.println("cliptest: bytes="+bytes);
    if(bytes!=compsize)report("wrong number of bytes!\n");
    for(int i=0;i<bytes;i++){
      if(buffer[i]!=(byte)comp[i]){
        for(int j=0;j<bytes;j++){
          System.err.println(j+": "+Integer.toHexString(buffer[j])+" "+
                             Integer.toHexString(comp[j]));
	}
	report("wrote incorrect value!\n");
      }
    }
System.err.println("bits: "+bits);
    r.readinit(buffer,bytes);
    for(int i=0;i<vals;i++){
      int tbit=(bits!=0)?bits:ilog(b[i]);
System.err.println(Integer.toHexString(b[i])+" tbit: "+tbit); 
      if(r.look(tbit)==-1){
        report("out of data!\n");
      }
      if(r.look(tbit)!=(b[i]&mask[tbit])){
        report(i+" looked at incorrect value! "+Integer.toHexString(r.look(tbit))+", "+Integer.toHexString(b[i]&mask[tbit])+":"+b[i]+" bit="+tbit);
      }
      if(tbit==1){
        if(r.look1()!=(b[i]&mask[tbit])){
  	  report("looked at single bit incorrect value!\n");
	}
      }
      if(tbit==1){
        if(r.read1()!=(b[i]&mask[tbit])){
	  report("read incorrect single bit value!\n");
	}
      }
      else{
	if(r.read(tbit)!=(b[i]&mask[tbit])){
	  report("read incorrect value!\n");
	}
      }
    }
    if(r.bytes()!=bytes){
      report("leftover bytes after read!\n");
    }
  }

  static int[] testbuffer1=
    {18,12,103948,4325,543,76,432,52,3,65,4,56,32,42,34,21,1,23,32,546,456,7,
       567,56,8,8,55,3,52,342,341,4,265,7,67,86,2199,21,7,1,5,1,4};
  static int test1size=43;

  static int[] testbuffer2=
    {216531625,1237861823,56732452,131,3212421,12325343,34547562,12313212,
       1233432,534,5,346435231,14436467,7869299,76326614,167548585,
       85525151,0,12321,1,349528352};
  static int test2size=21;

  static int[] large=
    {2136531625,2137861823,56732452,131,3212421,12325343,34547562,12313212,
       1233432,534,5,2146435231,14436467,7869299,76326614,167548585,
       85525151,0,12321,1,2146528352};

  static int[] testbuffer3=
    {1,0,14,0,1,0,12,0,1,0,0,0,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,1,1,1,1,0,0,1,
       0,1,30,1,1,1,0,0,1,0,0,0,12,0,11,0,1,0,0,1};
  static int test3size=56;

  static int onesize=33;
  static int[] one={146,25,44,151,195,15,153,176,233,131,196,65,85,172,47,40,
                    34,242,223,136,35,222,211,86,171,50,225,135,214,75,172,
                    223,4};

  static int twosize=6;
  static int[] two={61,255,255,251,231,29};

  static int threesize=54;
  static int[] three={169,2,232,252,91,132,156,36,89,13,123,176,144,32,254,
                      142,224,85,59,121,144,79,124,23,67,90,90,216,79,23,83,
                      58,135,196,61,55,129,183,54,101,100,170,37,127,126,10,
                      100,52,4,14,18,86,77,1};

  static int foursize=38;
  static int[] four={18,6,163,252,97,194,104,131,32,1,7,82,137,42,129,11,72,
                     132,60,220,112,8,196,109,64,179,86,9,137,195,208,122,169,
                     28,2,133,0,1};

  static int fivesize=45;
  static int[] five={169,2,126,139,144,172,30,4,80,72,240,59,130,218,73,62,
                     241,24,210,44,4,20,0,248,116,49,135,100,110,130,181,169,
                     84,75,159,2,1,0,132,192,8,0,0,18,22};

  static int sixsize=7;
  static int[] six={17,177,170,242,169,19,148};

  static Buffer o=new Buffer();
  static Buffer r=new Buffer();

  public static void main(String[] arg){
    byte[] buffer;
    int bytes;
//  o=new Buffer();
//  r=new Buffer();

    o.writeinit();

    System.err.print("\nSmall preclipped packing: ");
    cliptest(testbuffer1,test1size,0,one,onesize);
    System.err.print("ok.");

    System.err.print("\nNull bit call: ");
    cliptest(testbuffer3,test3size,0,two,twosize);
    System.err.print("ok.");

    System.err.print("\nLarge preclipped packing: ");
    cliptest(testbuffer2,test2size,0,three,threesize);
    System.err.print("ok.");

    System.err.print("\n32 bit preclipped packing: ");
    o.reset();
    for(int i=0;i<test2size;i++)
      o.write(large[i],32);
    buffer=o.buffer();
    bytes=o.bytes();


    r.readinit(buffer,bytes);
    for(int i=0;i<test2size;i++){
      if(r.look(32)==-1){
	report("out of data. failed!");
      }
      if(r.look(32)!=large[i]){
	System.err.print(r.look(32)+" != "+large[i]+" ("+
                         Integer.toHexString(r.look(32))+"!="+
                         Integer.toHexString(large[i])+")");
	report("read incorrect value!\n");
      }
      r.adv(32);
    }
    if(r.bytes()!=bytes)report("leftover bytes after read!\n");
    System.err.print("ok.");

    System.err.print("\nSmall unclipped packing: ");
    cliptest(testbuffer1,test1size,7,four,foursize);
    System.err.print("ok.");

    System.err.print("\nLarge unclipped packing: ");
    cliptest(testbuffer2,test2size,17,five,fivesize);
    System.err.print("ok.");

    System.err.print("\nSingle bit unclicpped packing: ");
    cliptest(testbuffer3,test3size,1,six,sixsize);
    System.err.print("ok.");

    System.err.print("\nTesting read past end: ");
    r.readinit("\0\0\0\0\0\0\0\0".getBytes(),8);
    for(int i=0;i<64;i++){
      if(r.read(1)!=0){
	System.err.print("failed; got -1 prematurely.\n");
	System.exit(1);
      }
    }

    if(r.look(1)!=-1 ||
       r.read(1)!=-1){
      System.err.print("failed; read past end without -1.\n");
      System.exit(1);
    }

    r.readinit("\0\0\0\0\0\0\0\0".getBytes(),8);
    if(r.read(30)!=0 || r.read(16)!=0){
      System.err.print("failed 2; got -1 prematurely.\n");
    System.exit(1);
    }

    if(r.look(18)!=0 ||
       r.look(18)!=0){
      System.err.print("failed 3; got -1 prematurely.\n");
      System.exit(1);
    }
    if(r.look(19)!=-1 ||
       r.look(19)!=-1){
      System.err.print("failed; read past end without -1.\n");
      System.exit(1);
    }
    if(r.look(32)!=-1 ||
       r.look(32)!=-1){
      System.err.print("failed; read past end without -1.\n");
      System.exit(1);
    }
    System.err.print("ok.\n\n");
  }
  */
}





