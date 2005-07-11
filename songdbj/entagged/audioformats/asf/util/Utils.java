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
package entagged.audioformats.asf.util;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.util.Calendar;
import java.util.GregorianCalendar;

import entagged.audioformats.asf.data.GUID;

/**
 * Some static Methods which are used in several Classes. <br>
 * 
 * @author Christian Laireiter
 */
public class Utils {

    /**
     * Stores the default line seperator of the current underlying system.
     */
    public final static String LINE_SEPARATOR = System
            .getProperty("line.separator");

    /**
     * Reads chars out of <code>raf</code> until <code>chars</code> is
     * filled.
     * 
     * @param chars
     *                   to be filled
     * @param raf
     *                   to be read
     * @throws IOException
     *                    read error, or file at end before <code>chars</code> is
     *                    filled.
     */
    public static void fillChars(char[] chars, RandomAccessFile raf)
            throws IOException {
        if (chars == null) {
            throw new IllegalArgumentException("Argument must not be null.");
        }
        for (int i = 0; i < chars.length; i++) {
            chars[i] = raf.readChar();
        }
    }

    /**
     * This method will create a byte[] at the size of <code>byteCount</code>
     * and insert the bytes of <code>value</code> (starting from lowset byte)
     * into it. <br>
     * You can easily create a Word (16-bit), DWORD (32-bit), QWORD (64 bit) out
     * of the value, ignoring the original type of value, since java
     * automatically performs transformations. <br>
     * <b>Warning: </b> This method works with unsigned numbers only.
     * 
     * @param value
     *                   The value to be written into the result.
     * @param byteCount
     *                   The number of bytes the array has got.
     * @return A byte[] with the size of <code>byteCount</code> containing the
     *               lower byte values of <code>value</code>.
     */
    public static byte[] getBytes(long value, int byteCount) {
        byte[] result = new byte[byteCount];
        for (int i = 0; i < result.length; i++) {
            result[i] = (byte) (value & 0xFF);
            value >>>= 8;
        }
        return result;
    }

    /**
     * Since date values in asf files are given in 100 ns steps since first
     * january of 1601 a little conversion must be done. <br>
     * This method converts a date given in described manner to a calendar.
     * 
     * @param fileTime
     *                   Time in 100ns since 1 jan 1601
     * @return Calendar holding the date representation.
     */
    public static GregorianCalendar getDateOf(BigInteger fileTime) {
        GregorianCalendar result = new GregorianCalendar(1601, 0, 1);
        // lose anything beyond milliseconds, because calendar can't handle
        // less value
        fileTime = fileTime.divide(new BigInteger("10000"));
        BigInteger maxInt = new BigInteger(String.valueOf(Integer.MAX_VALUE));
        while (fileTime.compareTo(maxInt) > 0) {
            result.add(Calendar.MILLISECOND, Integer.MAX_VALUE);
            fileTime = fileTime.subtract(maxInt);
        }
        result.add(Calendar.MILLISECOND, fileTime.intValue());
        return result;
    }

    /**
     * This method reads one byte from <code>raf</code> and creates an
     * unsigned value of it. <br>
     * 
     * @param raf
     *                   The file to read from.
     * @return next 7 bits as number.
     * @throws IOException
     *                    read errors.
     */
    public static int read7Bit(RandomAccessFile raf) throws IOException {
        int result = raf.read();
        return result & 127;
    }

    /**
     * This method reads 8 bytes, interprets them as an unsigned number and
     * creates a {@link BigInteger}
     * 
     * @param raf
     *                   Input source
     * @return 8 bytes unsigned number
     * @throws IOException
     *                    read errors.
     */
    public static BigInteger readBig64(RandomAccessFile raf) throws IOException {
        byte[] bytes = new byte[8];
        byte[] oa = new byte[8];
        raf.readFully(bytes);
        for (int i = 0; i < bytes.length; i++) {
            oa[7 - i] = bytes[i];
        }
        BigInteger result = new BigInteger(oa);
        return result;
    }

    /**
     * This method reads a UTF-16 String, which legth is given on the number of
     * characters it consits of. <br>
     * The filepointer of <code>raf</code> must be at the number of
     * characters. This number contains the terminating zero character (UINT16).
     * 
     * @param raf
     *                   Input source
     * @return String
     * @throws IOException
     *                    read errors
     */
    public static String readCharacterSizedString(RandomAccessFile raf)
            throws IOException {
        StringBuffer result = new StringBuffer();
        int strLen = readUINT16(raf);
        int character = raf.read();
        character |= raf.read() << 8;
        do {
            if (character != 0) {
                result.append((char) character);
                character = raf.read();
                character |= raf.read() << 8;
            }
        } while (character != 0 || (result.length() + 1) > strLen);
        if (strLen != (result.length() + 1)) {
            throw new IllegalStateException(
                    "Invalid Data for current interpretation");
        }
        return result.toString();
    }

    /**
     * This Method reads a GUID (which is a 16 byte long sequence) from the
     * given <code>raf</code> and creates a wrapper. <br>
     * <b>Warning </b>: <br>
     * There is no way of telling if a byte sequence is a guid or not. The next
     * 16 bytes will be interpreted as a guid, whether it is or not.
     * 
     * @param raf
     *                   Input source.
     * @return A class wrapping the guid.
     * @throws IOException
     *                    happens when the file ends before guid could be extracted.
     */
    public static GUID readGUID(RandomAccessFile raf) throws IOException {
        if (raf == null) {
            throw new IllegalArgumentException("Argument must not be null");
        }
        int[] binaryGuid = new int[GUID.GUID_LENGTH];
        for (int i = 0; i < binaryGuid.length; i++) {
            binaryGuid[i] = raf.read();
        }
        return new GUID(binaryGuid);
    }

    /**
     * @see #readUINT64(RandomAccessFile)
     * @param raf
     * @return number
     * @throws IOException
     */
    public static int readUINT16(RandomAccessFile raf) throws IOException {
        int result = raf.read();
        result |= raf.read() << 8;
        return result;
    }

    /**
     * @see #readUINT64(RandomAccessFile)
     * @param raf
     * @return number
     * @throws IOException
     */
    public static long readUINT32(RandomAccessFile raf) throws IOException {
        long result = 0;
        for (int i = 0; i <= 24; i += 8)
            result |= raf.read() << i;
        return result;
    }

    /**
     * Reads long as little endian.
     * 
     * @param raf
     *                   Data source
     * @return long value
     * @throws IOException
     *                    read error, or eof is reached before long is completed
     */
    public static long readUINT64(RandomAccessFile raf) throws IOException {
        long result = 0;
        for (int i = 0; i <= 56; i += 8)
            result |= raf.read() << i;
        return result;
    }

    /**
     * This method reads a UTF-16 encoded String, beginning with a 16-bit value
     * representing the number of bytes needed. The String is terminated with as
     * 16-bit ZERO. <br>
     * 
     * @param raf
     *                   Input source
     * @return read String.
     * @throws IOException
     *                    read errors.
     */
    public static String readUTF16LEStr(RandomAccessFile raf)
            throws IOException {
        int strLen = readUINT16(raf);
        byte[] buf = new byte[strLen];
        int read = raf.read(buf);
        if (read == buf.length) {
            /*
             * Check on zero termination
             */
            if (buf.length >= 2) {
                if (buf[buf.length - 1] == 0 && buf[buf.length - 2] == 0) {
                    byte[] copy = new byte[buf.length - 2];
                    System.arraycopy(buf, 0, copy, 0, buf.length - 2);
                    buf = copy;
                }
            }
            return new String(buf, "UTF-16LE");
        }
        throw new IllegalStateException(
                "Invalid Data for current interpretation");
    }

    /**
     * This method converts the given string into a byte[] in UTF-16LE encoding
     * and checks whether the length doesn't exceed 65535 bytes. <br>
     * 
     * @param value
     *                   The string to check.
     * @throws IllegalArgumentException
     *                    If byte representation takes more than 65535 bytes.
     */
    public static void checkStringLengthNullSafe(String value)
            throws IllegalArgumentException {
        if (value != null) {
            try {
                byte[] tmp = value.getBytes("UTF-16LE");
                if (tmp.length > 65533) {
                    throw new IllegalArgumentException(
                            "\"UTF-16LE\" representation exceeds 65535 bytes."
                                    + " (Including zero term character)");
                }
            } catch (UnsupportedEncodingException e) {
                e.printStackTrace();
            }
        }
    }
}