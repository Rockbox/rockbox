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

public class Page{
  private static int[] crc_lookup=new int[256];
  static {
    for(int i=0; i<crc_lookup.length; i++){
      crc_lookup[i]=crc_entry(i);
    }
  }

  private static int crc_entry(int index){
    int r=index<<24;
    for(int i=0; i<8; i++){
      if((r& 0x80000000)!=0){
        r=(r << 1)^0x04c11db7; /* The same as the ethernet generator
			          polynomial, although we use an
				  unreflected alg and an init/final
				  of 0, not 0xffffffff */
      }
      else{
	r<<=1;
      }
    }
    return(r&0xffffffff);
  }

  public byte[] header_base;
  public int header;
  public int header_len;
  public byte[] body_base;
  public int body;
  public int body_len;

  int version(){
    return header_base[header+4]&0xff;
  }
  int continued(){
    return (header_base[header+5]&0x01);
  }
  public int bos(){
    return (header_base[header+5]&0x02);
  }
  public int eos(){
    return (header_base[header+5]&0x04);
  }
  public long granulepos(){
    long foo=header_base[header+13]&0xff;
    foo=(foo<<8)|(header_base[header+12]&0xff);
    foo=(foo<<8)|(header_base[header+11]&0xff);
    foo=(foo<<8)|(header_base[header+10]&0xff);
    foo=(foo<<8)|(header_base[header+9]&0xff);
    foo=(foo<<8)|(header_base[header+8]&0xff);
    foo=(foo<<8)|(header_base[header+7]&0xff);
    foo=(foo<<8)|(header_base[header+6]&0xff);
    return(foo);
  }
  public int serialno(){
    return (header_base[header+14]&0xff)|
           ((header_base[header+15]&0xff)<<8)|
           ((header_base[header+16]&0xff)<<16)|
           ((header_base[header+17]&0xff)<<24);
  }
  int pageno(){
    return (header_base[header+18]&0xff)|
           ((header_base[header+19]&0xff)<<8)|
           ((header_base[header+20]&0xff)<<16)|
           ((header_base[header+21]&0xff)<<24);
  }

  void checksum(){
    int crc_reg=0;

//  for(int i=0;i<header_len;i++){
//    System.err.println("chksum: "+Integer.toHexString(header_base[header+i]&0xff));
//  }
    
    for(int i=0;i<header_len;i++){
      crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg>>>24)&0xff)^(header_base[header+i]&0xff)];
    }
    for(int i=0;i<body_len;i++){
      crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg>>>24)&0xff)^(body_base[body+i]&0xff)];
    }
    header_base[header+22]=(byte)crc_reg/*&0xff*/;
    header_base[header+23]=(byte)(crc_reg>>>8)/*&0xff*/;
    header_base[header+24]=(byte)(crc_reg>>>16)/*&0xff*/;
    header_base[header+25]=(byte)(crc_reg>>>24)/*&0xff*/;
  }
  public Page copy(){
    return copy(new Page());
  }
  public Page copy(Page p){
    byte[] tmp=new byte[header_len];
    System.arraycopy(header_base, header, tmp, 0, header_len);
    p.header_len=header_len;
    p.header_base=tmp;
    p.header=0;
    tmp=new byte[body_len];
    System.arraycopy(body_base, body, tmp, 0, body_len);
    p.body_len=body_len;
    p.body_base=tmp;
    p.body=0;
    return p;
  }
  /*
  // TEST
  static StreamState os_en, os_de;
  static SyncState oy;
  void check_page(byte[] data_base, int data, int[] _header){
    // Test data
    for(int j=0;j<body_len;j++)
      if(body_base[body+j]!=data_base[data+j]){
	System.err.println("body data mismatch at pos "+j+": "+data_base[data+j]+"!="+body_base[body+j]+"!\n");
      System.exit(1);
    }

    // Test header
    for(int j=0;j<header_len;j++){
      if((header_base[header+j]&0xff)!=_header[j]){
	System.err.println("header content mismatch at pos "+j);
	for(int jj=0;jj<_header[26]+27;jj++)
	  System.err.print(" ("+jj+")"+Integer.toHexString(_header[jj])+":"+Integer.toHexString(header_base[header+jj]));
	System.err.println("");
	System.exit(1);
      }
    }
    if(header_len!=_header[26]+27){
      System.err.print("header length incorrect! ("+header_len+"!="+(_header[26]+27)+")");
      System.exit(1);
    }
  }

  void print_header(){
    System.err.println("\nHEADER:");
    System.err.println("  capture: "+
		       (header_base[header+0]&0xff)+" "+
		       (header_base[header+1]&0xff)+" "+
		       (header_base[header+2]&0xff)+" "+
		       (header_base[header+3]&0xff)+" "+
                       " version: "+(header_base[header+4]&0xff)+"  flags: "+
		       (header_base[header+5]&0xff));
    System.err.println("  pcmpos: "+
		       (((header_base[header+9]&0xff)<<24)|
			((header_base[header+8]&0xff)<<16)|
			((header_base[header+7]&0xff)<<8)|
			((header_base[header+6]&0xff)))+
		       "  serialno: "+
		       (((header_base[header+17]&0xff)<<24)|
			((header_base[header+16]&0xff)<<16)|
			((header_base[header+15]&0xff)<<8)|
			((header_base[header+14]&0xff)))+
		       "  pageno: "+
		       (((header_base[header+21]&0xff)<<24)|
			((header_base[header+20]&0xff)<<16)|
			((header_base[header+19]&0xff)<<8)|
			((header_base[header+18]&0xff))));

    System.err.println("  checksum: "+
		       (header_base[header+22]&0xff)+":"+
		       (header_base[header+23]&0xff)+":"+
		       (header_base[header+24]&0xff)+":"+
		       (header_base[header+25]&0xff)+"\n  segments: "+
		       (header_base[header+26]&0xff)+" (");
    for(int j=27;j<header_len;j++){
      System.err.println((header_base[header+j]&0xff)+" ");
    }
    System.err.println(")\n");
  }

  void copy_page(){
    byte[] tmp=new byte[header_len];
    System.arraycopy(header_base, header, tmp, 0, header_len);
    header_base=tmp;
    header=0;
    tmp=new byte[body_len];
    System.arraycopy(body_base, body, tmp, 0, body_len);
    body_base=tmp;
    body=0;
  }

  static void test_pack(int[] pl, int[][] headers){
    byte[] data=new byte[1024*1024]; // for scripted test cases only
    int inptr=0;
    int outptr=0;
    int deptr=0;
    int depacket=0;
    int pcm_pos=7;
    int packets,pageno=0,pageout=0;
    int eosflag=0;
    int bosflag=0;

    os_en.reset();
    os_de.reset();
    oy.reset();

    for(packets=0;;packets++){
      if(pl[packets]==-1)break;
    }

    for(int i=0;i<packets;i++){
      // construct a test packet
      Packet op=new Packet();
      int len=pl[i];
      op.packet_base=data;
      op.packet=inptr;
      op.bytes=len;
      op.e_o_s=(pl[i+1]<0?1:0);
      op.granulepos=pcm_pos;

      pcm_pos+=1024;

      for(int j=0;j<len;j++){
	data[inptr++]=(byte)(i+j);
      }

      // submit the test packet
      os_en.packetin(op);

      // retrieve any finished pages
      {
	Page og=new Page();
      
	while(os_en.pageout(og)!=0){
	  // We have a page.  Check it carefully
	  //System.err.print(pageno+", ");
	  if(headers[pageno]==null){
	    System.err.println("coded too many pages!");
	    System.exit(1);
	  }
	  og.check_page(data, outptr, headers[pageno]);

	  outptr+=og.body_len;
	  pageno++;

//System.err.println("1# pageno="+pageno+", pageout="+pageout);

	  // have a complete page; submit it to sync/decode
	  
	  {
	    Page og_de=new Page();
	    Packet op_de=new Packet();
	    int index=oy.buffer(og.header_len+og.body_len);
	    byte[] buf=oy.data;
           System.arraycopy(og.header_base, og.header, buf, index, og.header_len);
           System.arraycopy(og.body_base, og.body, buf, index+og.header_len, og.body_len);
	    oy.wrote(og.header_len+og.body_len);

//System.err.println("2# pageno="+pageno+", pageout="+pageout);

	    while(oy.pageout(og_de)>0){
	      // got a page.  Happy happy.  Verify that it's good.
	    
	      og_de.check_page(data, deptr, headers[pageout]);
	      deptr+=og_de.body_len;
	      pageout++;

	      // submit it to deconstitution
	      os_de.pagein(og_de);

	      // packets out?
	      while(os_de.packetout(op_de)>0){
	      
 	        // verify the packet!
	        // check data
		boolean check=false;
		for(int ii=0; ii<op_de.bytes; ii++){
		  if(data[depacket+ii]!=op_de.packet_base[op_de.packet+ii]){
		    check=true;
		    break;
		  }
		}
		if(check){
		  System.err.println("packet data mismatch in decode! pos="+
				     depacket);
		  System.exit(1);
		}

	        // check bos flag
		if(bosflag==0 && op_de.b_o_s==0){
		  System.err.println("b_o_s flag not set on packet!");
		  System.exit(1);
		}
		if(bosflag!=0 && op_de.b_o_s!=0){
		  System.err.println("b_o_s flag incorrectly set on packet!");
		  System.exit(1);
		}
	      
		bosflag=1;
		depacket+=op_de.bytes;
	      
	        // check eos flag
		if(eosflag!=0){
		  System.err.println("Multiple decoded packets with eos flag!");
		  System.exit(1);
		}

		if(op_de.e_o_s!=0)eosflag=1;

		// check pcmpos flag
		if(op_de.granulepos!=-1){
		  System.err.print(" pcm:"+op_de.granulepos+" ");
		}
	      }
	    }
	  }
	}
      }
    }
    //free(data);
    if(headers[pageno]!=null){
      System.err.println("did not write last page!");
      System.exit(1);
    }
    if(headers[pageout]!=null){
      System.err.println("did not decode last page!");
      System.exit(1);
    }
    if(inptr!=outptr){
      System.err.println("encoded page data incomplete!");
      System.exit(1);
    }
    if(inptr!=deptr){
      System.err.println("decoded page data incomplete!");
      System.exit(1);
    }
    if(inptr!=depacket){
      System.err.println("decoded packet data incomplete!");
      System.exit(1);
    }
    if(eosflag==0){
      System.err.println("Never got a packet with EOS set!");
    }
    System.err.println("ok.");
  }

  static void error(){
    System.err.println("error!");
    System.exit(1);
  }
  public static void main(String[] arg){

    os_en=new StreamState(0x04030201);
    os_de=new StreamState(0x04030201);

    oy=new SyncState();

    // Exercise each code path in the framing code.  Also verify that
    // the checksums are working.

    {
      // 17 only
      int[] packets={17, -1};
      int[] head1={0x4f,0x67,0x67,0x53,0,0x06,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x15,0xed,0xec,0x91,
		   1,
		   17};
      int[][] headret={head1, null};
    
      System.err.print("testing single page encoding... ");
      test_pack(packets,headret);
    }

    {
      // 17, 254, 255, 256, 500, 510, 600 byte, pad
      int[] packets={17, 254, 255, 256, 500, 510, 600, -1};
      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x59,0x10,0x6c,0x2c,
		   1,
                   17};
      int[] head2={0x4f,0x67,0x67,0x53,0,0x04,
		   0x07,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x89,0x33,0x85,0xce,
		   13,
		   254,255,0,255,1,255,245,255,255,0,
		   255,255,90};
      int[][] headret={head1,head2,null};

      System.err.print("testing basic page encoding... ");
      test_pack(packets,headret);
    }

    {
      // nil packets; beginning,middle,end
      int[] packets={0,17, 254, 255, 0, 256, 0, 500, 510, 600, 0, -1};

      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xff,0x7b,0x23,0x17,
		   1,
		   0};
      int[] head2={0x4f,0x67,0x67,0x53,0,0x04,
		   0x07,0x28,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x5c,0x3f,0x66,0xcb,
		   17,
		   17,254,255,0,0,255,1,0,255,245,255,255,0,
		   255,255,90,0};
      int[][] headret={head1,head2,null};

      System.err.print("testing basic nil packets... ");
      test_pack(packets,headret);
    }

    {
      // large initial packet
      int[] packets={4345,259,255,-1};

      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x01,0x27,0x31,0xaa,
		   18,
		   255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255,255,10};

      int[] head2={0x4f,0x67,0x67,0x53,0,0x04,
		   0x07,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x7f,0x4e,0x8a,0xd2,
		   4,
		   255,4,255,0};
      int[][] headret={head1,head2,null};

      System.err.print("testing initial-packet lacing > 4k... ");
      test_pack(packets,headret);
    }

    {
      // continuing packet test
      int[] packets={0,4345,259,255,-1};

      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xff,0x7b,0x23,0x17,
		   1,
		   0};

      int[] head2={0x4f,0x67,0x67,0x53,0,0x00,
		   0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x34,0x24,0xd5,0x29,
		   17,
		   255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255,255};

      int[] head3={0x4f,0x67,0x67,0x53,0,0x05,
		   0x07,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,2,0,0,0,
		   0xc8,0xc3,0xcb,0xed,
		   5,
		   10,255,4,255,0};
      int[][] headret={head1,head2,head3,null};

      System.err.print("testing single packet page span... ");
      test_pack(packets,headret);
    }

    // page with the 255 segment limit
    {

      int[] packets={0,10,10,10,10,10,10,10,10,
		     10,10,10,10,10,10,10,10,
		     10,10,10,10,10,10,10,10,
		     10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,50,-1};

      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xff,0x7b,0x23,0x17,
		   1,
		   0};

      int[] head2={0x4f,0x67,0x67,0x53,0,0x00,
		   0x07,0xfc,0x03,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0xed,0x2a,0x2e,0xa7,
		   255,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10};

      int[] head3={0x4f,0x67,0x67,0x53,0,0x04,
		   0x07,0x00,0x04,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,2,0,0,0,
		   0x6c,0x3b,0x82,0x3d,
		   1,
		   50};
      int[][] headret={head1,head2,head3,null};

      System.err.print("testing max packet segments... ");
      test_pack(packets,headret);
    }

    {
      // packet that overspans over an entire page

      int[] packets={0,100,9000,259,255,-1};

      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xff,0x7b,0x23,0x17,
		   1,
		   0};

      int[] head2={0x4f,0x67,0x67,0x53,0,0x00,
		   0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x3c,0xd9,0x4d,0x3f,
		   17,
		   100,255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255};

      int[] head3={0x4f,0x67,0x67,0x53,0,0x01,
		   0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,2,0,0,0,
		   0xbd,0xd5,0xb5,0x8b,
		   17,
		   255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255,255};

      int[] head4={0x4f,0x67,0x67,0x53,0,0x05,
		   0x07,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,3,0,0,0,
		   0xef,0xdd,0x88,0xde,
		   7,
		   255,255,75,255,4,255,0};
      int[][] headret={head1,head2,head3,head4,null};

      System.err.print("testing very large packets... ");
      test_pack(packets,headret);
    }

    {
      // term only page.  why not?

      int[] packets={0,100,4080,-1};

      int[] head1={0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xff,0x7b,0x23,0x17,
		   1,
		   0};

      int[] head2={0x4f,0x67,0x67,0x53,0,0x00,
		   0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x3c,0xd9,0x4d,0x3f,
		   17,
		   100,255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255};

      int[] head3={0x4f,0x67,0x67,0x53,0,0x05,
		   0x07,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,2,0,0,0,
		   0xd4,0xe0,0x60,0xe5,
		   1,0};

      int[][] headret={head1,head2,head3,null};

      System.err.print("testing zero data page (1 nil packet)... ");
      test_pack(packets,headret);
    }

    {
      // build a bunch of pages for testing
      byte[] data=new byte[1024*1024];
      int[] pl={0,100,4079,2956,2057,76,34,912,0,234,1000,1000,1000,300,-1};
      int inptr=0;
      Page[] og=new Page[5];
      for(int i=0; i<5; i++){
	og[i]=new Page();
      }
    
      os_en.reset();

      for(int i=0;pl[i]!=-1;i++){
	Packet op=new Packet();
	int len=pl[i];
      
	op.packet_base=data;
	op.packet=inptr;
	op.bytes=len;
	op.e_o_s=(pl[i+1]<0?1:0);
	op.granulepos=(i+1)*1000;
	
	for(int j=0;j<len;j++)data[inptr++]=(byte)(i+j);
	os_en.packetin(op);
      }    

//    free(data);

      // retrieve finished pages
      for(int i=0;i<5;i++){
	if(os_en.pageout(og[i])==0){
	  System.err.print("Too few pages output building sync tests!\n");
	  System.exit(1);
	}
	og[i].copy_page();
      }

      // Test lost pages on pagein/packetout: no rollback
      {
	Page temp=new Page();
	Packet test=new Packet();

	System.err.print("Testing loss of pages... ");

	oy.reset();
	os_de.reset();
	for(int i=0;i<5;i++){
	  int index=oy.buffer(og[i].header_len);
	  System.arraycopy(og[i].header_base, og[i].header,
			   oy.data, index, og[i].header_len);
	  oy.wrote(og[i].header_len);
	  index=oy.buffer(og[i].body_len);
	  System.arraycopy(og[i].body_base, og[i].body,
			   oy.data, index, og[i].body_len);
	  oy.wrote(og[i].body_len);
	}

	oy.pageout(temp);
	os_de.pagein(temp);
	oy.pageout(temp);
	os_de.pagein(temp);
	oy.pageout(temp);

        // skip
	oy.pageout(temp);
	os_de.pagein(temp);

        // do we get the expected results/packets?
      
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(0,0,0);
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(100,1,-1);
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(4079,2,3000);
	if(os_de.packetout(test)!=-1){
	  System.err.println("Error: loss of page did not return error");
	  System.exit(1);
	}
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(76,5,-1);
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(34,6,-1);
	System.err.println("ok.");
      }

      // Test lost pages on pagein/packetout: rollback with continuation
      {
	Page temp=new Page();
	Packet test=new Packet();

	System.err.print("Testing loss of pages (rollback required)... ");

	oy.reset();
	os_de.reset();
	for(int i=0;i<5;i++){
	  int index=oy.buffer(og[i].header_len);
	  System.arraycopy(og[i].header_base, og[i].header,
			   oy.data, index, og[i].header_len);
	  oy.wrote(og[i].header_len);
	  index=oy.buffer(og[i].body_len);
	  System.arraycopy(og[i].body_base, og[i].body,
			   oy.data, index, og[i].body_len);
	  oy.wrote(og[i].body_len);
	}

	oy.pageout(temp);
	os_de.pagein(temp);
	oy.pageout(temp);
	os_de.pagein(temp);
	oy.pageout(temp);
	os_de.pagein(temp);
	oy.pageout(temp);
        // skip
	oy.pageout(temp);
	os_de.pagein(temp);

        // do we get the expected results/packets?
      
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(0,0,0);
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(100,1,-1);
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(4079,2,3000);
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(2956,3,4000);
	if(os_de.packetout(test)!=-1){
	  System.err.println("Error: loss of page did not return error");
	  System.exit(1);
	}
	if(os_de.packetout(test)!=1)error();
	test.checkpacket(300,13,14000);
	System.err.println("ok.");
      }

      // the rest only test sync
      {
    	Page og_de=new Page();
        // Test fractional page inputs: incomplete capture
    	System.err.print("Testing sync on partial inputs... ");
    	oy.reset();
    	int index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header, 
    			 oy.data, index, 3);
    	oy.wrote(3);
    	if(oy.pageout(og_de)>0)error();
      
        // Test fractional page inputs: incomplete fixed header
    	index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header+3, 
    			 oy.data, index, 20);
    
    	oy.wrote(20);
    	if(oy.pageout(og_de)>0)error();
    
        // Test fractional page inputs: incomplete header
    	index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header+23, 
    			 oy.data, index, 5);
    	oy.wrote(5);
    	if(oy.pageout(og_de)>0)error();
    
        // Test fractional page inputs: incomplete body
    	index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header+28, 
    			 oy.data, index, og[1].header_len-28);
    	oy.wrote(og[1].header_len-28);
    	if(oy.pageout(og_de)>0)error();
    
    	index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body,
    			 oy.data, index, 1000);
    	oy.wrote(1000);
    	if(oy.pageout(og_de)>0)error();

    	index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body+1000,
    			 oy.data, index, og[1].body_len-1000);
    	oy.wrote(og[1].body_len-1000);
    	if(oy.pageout(og_de)<=0)error();
    	System.err.println("ok.");
      }

      // Test fractional page inputs: page + incomplete capture
      {
    	Page og_de=new Page();
    	System.err.print("Testing sync on 1+partial inputs... ");
    	oy.reset(); 

    	int index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header,
    			 oy.data, index, og[1].header_len);
    	oy.wrote(og[1].header_len);

    	index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body,
    			 oy.data, index, og[1].body_len);
    	oy.wrote(og[1].body_len);

    	index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header,
    			 oy.data, index, 20);
    	oy.wrote(20);
    	if(oy.pageout(og_de)<=0)error();
    	if(oy.pageout(og_de)>0)error();

    	index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header+20,
    			 oy.data, index, og[1].header_len-20);
    	oy.wrote(og[1].header_len-20);
    	index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body,
    			 oy.data, index, og[1].body_len);
    	     
    	oy.wrote(og[1].body_len);
    	if(oy.pageout(og_de)<=0)error();
    
    	System.err.println("ok.");
      }

//    //    //    //    //    //    //    //    //    
      // Test recapture: garbage + page
      {
    	Page og_de=new Page();
    	System.err.print("Testing search for capture... ");
    	oy.reset(); 
      
        // 'garbage'
    	int index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body,
    			 oy.data, index, og[1].body_len);
    	oy.wrote(og[1].body_len);

    	index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header,
    			 oy.data, index, og[1].header_len);
    	oy.wrote(og[1].header_len);

    	index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body,
    			 oy.data, index, og[1].body_len);
    	oy.wrote(og[1].body_len);

    	index=oy.buffer(og[2].header_len);
    	System.arraycopy(og[2].header_base, og[2].header,
    			 oy.data, index, 20);

    	oy.wrote(20);
    	if(oy.pageout(og_de)>0)error();
    	if(oy.pageout(og_de)<=0)error();
    	if(oy.pageout(og_de)>0)error();

    	index=oy.buffer(og[2].header_len);
    	System.arraycopy(og[2].header_base, og[2].header+20,
    			 oy.data, index, og[2].header_len-20);
    	oy.wrote(og[2].header_len-20);
    	index=oy.buffer(og[2].body_len);
    	System.arraycopy(og[2].body_base, og[2].body,
    			 oy.data, index, og[2].body_len);
    	oy.wrote(og[2].body_len);
    	if(oy.pageout(og_de)<=0)error();

    	System.err.println("ok.");
      }

      // Test recapture: page + garbage + page
      {
    	Page og_de=new Page();
    	System.err.print("Testing recapture... ");
    	oy.reset(); 

    	int index=oy.buffer(og[1].header_len);
    	System.arraycopy(og[1].header_base, og[1].header,
    			 oy.data, index, og[1].header_len);
    	oy.wrote(og[1].header_len);

    	index=oy.buffer(og[1].body_len);
    	System.arraycopy(og[1].body_base, og[1].body,
    			 oy.data, index, og[1].body_len);
    	oy.wrote(og[1].body_len);

    	index=oy.buffer(og[2].header_len);
    	System.arraycopy(og[2].header_base, og[2].header,
    			 oy.data, index, og[2].header_len);
    	oy.wrote(og[2].header_len);

    	index=oy.buffer(og[2].header_len);
    	System.arraycopy(og[2].header_base, og[2].header,
    			 oy.data, index, og[2].header_len);
    	oy.wrote(og[2].header_len);

    	if(oy.pageout(og_de)<=0)error();

    	index=oy.buffer(og[2].body_len);
    	System.arraycopy(og[2].body_base, og[2].body,
    			 oy.data, index, og[2].body_len-5);
    	oy.wrote(og[2].body_len-5);

    	index=oy.buffer(og[3].header_len);
    	System.arraycopy(og[3].header_base, og[3].header,
    			 oy.data, index, og[3].header_len);
    	oy.wrote(og[3].header_len);

    	index=oy.buffer(og[3].body_len);
    	System.arraycopy(og[3].body_base, og[3].body,
    			 oy.data, index, og[3].body_len);
    	oy.wrote(og[3].body_len);

    	if(oy.pageout(og_de)>0)error();
    	if(oy.pageout(og_de)<=0)error();

    	System.err.println("ok.");
      }
    }    
    //return(0);
  }
  */
}
