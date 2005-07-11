/*
 *  ********************************************************************   **
 *  Copyright notice                                                       **
 *  **																	   **
 *  (c) 2003 Entagged Developpement Team				                   **
 *  http://www.sourceforge.net/projects/entagged                           **
 *  **																	   **
 *  All rights reserved                                                    **
 *  **																	   **
 *  This script is part of the Entagged project. The Entagged 			   **
 *  project is free software; you can redistribute it and/or modify        **
 *  it under the terms of the GNU General Public License as published by   **
 *  the Free Software Foundation; either version 2 of the License, or      **
 *  (at your option) any later version.                                    **
 *  **																	   **
 *  The GNU General Public License can be found at                         **
 *  http://www.gnu.org/copyleft/gpl.html.                                  **
 *  **																	   **
 *  This copyright notice MUST APPEAR in all copies of the file!           **
 *  ********************************************************************
 */
package entagged.audioformats;

import java.util.*;

public class EncodingInfo {
	
    private Hashtable content;
	
	public EncodingInfo() {
		content = new Hashtable(6);
		content.put("BITRATE", new Integer(-1) );
		content.put("CHANNB", new Integer(-1) );
		content.put("TYPE", "");
		content.put("INFOS", "");
		content.put("SAMPLING", new Integer(-1) );
		content.put("LENGTH", new Integer(-1) );
		content.put("VBR", new Boolean(true) );
	}
	
	//Sets the bitrate in KByte/s
	public void setBitrate( int bitrate ) {
		content.put("BITRATE", new Integer(bitrate) );
	}
	//Sets the number of channels
	public void setChannelNumber( int chanNb ) {
		content.put("CHANNB", new Integer(chanNb) );
	}
	//Sets the type of the encoding, this is a bit format specific. eg:Layer I/II/II
	public void setEncodingType( String encodingType ) {
		content.put("TYPE", encodingType );
	}
	//A string contianing anything else that might be interesting
	public void setExtraEncodingInfos( String infos ) {
		content.put("INFOS", infos );
	}
	//Sets the Sampling rate in Hz
	public void setSamplingRate( int samplingRate ) {
		content.put("SAMPLING", new Integer(samplingRate) );
	}
	//Sets the length of the song in seconds
	public void setLength( int length ) {
		content.put("LENGTH", new Integer(length) );
	}
	//Is the song vbr or not ?
	public void setVbr( boolean b ) {
		content.put("VBR", new Boolean(b) );
	}
	
	
	//returns the bitrate in KByte/s
	public int getBitrate() {
		return ((Integer) content.get("BITRATE")).intValue();
	}
	//Returns the number of channels
	public int getChannelNumber() {
		return ((Integer) content.get("CHANNB")).intValue();
	}
	//returns the encoding type.
	public String getEncodingType() {
		return (String) content.get("TYPE");
	}
	//returns a string with misc. information about the encoding
	public String getExtraEncodingInfos() {
		return (String) content.get("INFOS");
	}
	//returns the sample rate in Hz
	public int getSamplingRate() {
		return ((Integer) content.get("SAMPLING")).intValue();
	}
	//Returns the length of the song in seconds
	public int getLength() {
		return ((Integer) content.get("LENGTH")).intValue();
	}
	//Is the song vbr ?
	public boolean isVbr() {
		return ((Boolean) content.get("VBR")).booleanValue();
	}
	
	//Pretty prints this encoding info
	public String toString() {
		StringBuffer out = new StringBuffer(50);
		out.append("Encoding infos content:\n");
		Enumeration en = content.keys();
		while(en.hasMoreElements()) {
          Object key = en.nextElement();
		  Object val = content.get(key);
		  out.append("\t");
		  out.append(key);
		  out.append(" : ");
		  out.append(val);
		  out.append("\n");
		}
		return out.toString().substring(0,out.length()-1);
	}
}
