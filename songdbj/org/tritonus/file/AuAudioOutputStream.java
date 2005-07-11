/*
 *	AuAudioOutputStream.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000,2001 by Florian Bomers <http://www.bomers.de>
 *  Copyright (c) 1999 by Matthias Pfisterer
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as published
 *   by the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
|<---            this code is formatted to fit into 80 columns             --->|
*/

package org.tritonus.sampled.file;

import java.io.IOException;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioSystem;
import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.TAudioOutputStream;
import org.tritonus.share.sampled.file.TDataOutputStream;



/**
 * AudioOutputStream for AU files.
 *
 * @author Florian Bomers
 * @author Matthias Pfisterer
 */

public class AuAudioOutputStream extends TAudioOutputStream {

	private static String description="Created by Tritonus";

	/**
	* Writes a null-terminated ascii string s to f.
	* The total number of bytes written is aligned on a 2byte boundary.
	* @exception IOException Write error.
	*/
	protected static void writeText(TDataOutputStream dos, String s) throws IOException {
		if (s.length()>0) {
			dos.writeBytes(s);
			dos.writeByte(0);  // pour terminer le texte
			if ((s.length() % 2)==0) {
				// ajout d'un zero pour faire la longeur pair
				dos.writeByte(0);
			}
		}
	}

	/**
	* Returns number of bytes that have to written for string s (with alignment)
	*/
	protected static int getTextLength(String s) {
		if (s.length()==0) {
			return 0;
		} else {
			return (s.length()+2) & 0xFFFFFFFE;
		}
	}

	public AuAudioOutputStream(AudioFormat audioFormat,
	                           long lLength,
	                           TDataOutputStream dataOutputStream) {
		// if length exceeds 2GB, set the length field to NOT_SPECIFIED
		super(audioFormat,
		      lLength>0x7FFFFFFFl?AudioSystem.NOT_SPECIFIED:lLength,
		      dataOutputStream,
		      lLength == AudioSystem.NOT_SPECIFIED && dataOutputStream.supportsSeek());
	}

	protected void writeHeader() throws IOException {
		if (TDebug.TraceAudioOutputStream) {
			TDebug.out("AuAudioOutputStream.writeHeader(): called.");
		}
		AudioFormat		format = getFormat();
		long			lLength = getLength();
		TDataOutputStream	dos = getDataOutputStream();
		if (TDebug.TraceAudioOutputStream) {
		    TDebug.out("AuAudioOutputStream.writeHeader(): AudioFormat: " + format);
		    TDebug.out("AuAudioOutputStream.writeHeader(): length: " + lLength);
		}

		dos.writeInt(AuTool.AU_HEADER_MAGIC);
		dos.writeInt(AuTool.DATA_OFFSET+getTextLength(description));
		dos.writeInt((lLength!=AudioSystem.NOT_SPECIFIED)?((int) lLength):AuTool.AUDIO_UNKNOWN_SIZE);
		dos.writeInt(AuTool.getFormatCode(format));
		dos.writeInt((int) format.getSampleRate());
		dos.writeInt(format.getChannels());
		writeText(dos, description);
	}

	protected void patchHeader() throws IOException {
		TDataOutputStream	tdos = getDataOutputStream();
		tdos.seek(0);
		setLengthFromCalculatedLength();
		writeHeader();
	}
}

/*** AuAudioOutputStream.java ***/
