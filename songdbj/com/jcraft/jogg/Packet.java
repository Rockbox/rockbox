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

public class Packet{
  public byte[] packet_base;
  public int packet;
  public int bytes;
  public int b_o_s;
  public int e_o_s;

  public long granulepos;

  public long packetno; // sequence number for decode; the framing
                       // knows where there's a hole in the data,
                       // but we need coupling so that the codec
                       // (which is in a seperate abstraction
                       // layer) also knows about the gap

  /*
  // TEST
  static int sequence=0;
  static int lastno=0;
  void checkpacket(int len, int no, int pos){
    if(bytes!=len){
      System.err.println("incorrect packet length!");
      System.exit(1);
    }
    if(granulepos!=pos){
      System.err.println("incorrect packet position!");
      System.exit(1);
    }

    // packet number just follows sequence/gap; adjust the input number
    // for that
    if(no==0){
      sequence=0;
    }
    else{
      sequence++;
      if(no>lastno+1)
	sequence++;
    }
    lastno=no;
    if(packetno!=sequence){
     System.err.println("incorrect packet sequence "+packetno+" != "+sequence);
      System.exit(1);
    }

    // Test data
    for(int j=0;j<bytes;j++){
      if((packet_base[packet+j]&0xff)!=((j+no)&0xff)){
	System.err.println("body data mismatch at pos "+ j+": "+(packet_base[packet+j]&0xff)+"!="+((j+no)&0xff)+"!\n");
	System.exit(1);
      }
    }
  }
  */
}
