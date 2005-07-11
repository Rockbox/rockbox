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
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.vorbis;

import java.io.*;

import java.util.*;

import de.jarnbjo.util.io.BitInputStream;

/**
 */

public class CommentHeader {

   public static final String TITLE = "TITLE";
   public static final String ARTIST = "ARTIST";
   public static final String ALBUM = "ALBUM";
   public static final String TRACKNUMBER = "TRACKNUMBER";
   public static final String VERSION = "VERSION";
   public static final String PERFORMER = "PERFORMER";
   public static final String COPYRIGHT = "COPYRIGHT";
   public static final String LICENSE = "LICENSE";
   public static final String ORGANIZATION = "ORGANIZATION";
   public static final String DESCRIPTION = "DESCRIPTION";
   public static final String GENRE = "GENRE";
   public static final String DATE = "DATE";
   public static final String LOCATION = "LOCATION";
   public static final String CONTACT = "CONTACT";
   public static final String ISRC = "ISRC";

   private String vendor;
   private HashMap comments=new HashMap();
   private boolean framingBit;

   private static final long HEADER = 0x736962726f76L; // 'vorbis'

   public CommentHeader(BitInputStream source) throws VorbisFormatException, IOException {
      if(source.getLong(48)!=HEADER) {
         throw new VorbisFormatException("The identification header has an illegal leading.");
      }

      vendor=getString(source);

      int ucLength=source.getInt(32);

      for(int i=0; i<ucLength; i++) {
         String comment=getString(source);
         int ix=comment.indexOf('=');
         String key=comment.substring(0, ix);
         String value=comment.substring(ix+1);
         //comments.put(key, value);
         addComment(key, value);
      }

      framingBit=source.getInt(8)!=0;
   }

   private void addComment(String key, String value) {
      key = key.toUpperCase();      // Comment keys are case insensitive
      ArrayList al=(ArrayList)comments.get(key);
      if(al==null) {
         al=new ArrayList();
         comments.put(key, al);
      }
      al.add(value);
   }

   public String getVendor() {
      return vendor;
   }

   public String getComment(String key) {
      ArrayList al=(ArrayList)comments.get(key);
      return al==null?(String)null:(String)al.get(0);
   }

   public String[] getComments(String key) {
      ArrayList al=(ArrayList)comments.get(key);
      return al==null?new String[0]:(String[])al.toArray(new String[al.size()]);
   }

   public String getTitle() {
      return getComment(TITLE);
   }

   public String[] getTitles() {
      return getComments(TITLE);
   }

   public String getVersion() {
      return getComment(VERSION);
   }

   public String[] getVersions() {
      return getComments(VERSION);
   }

   public String getAlbum() {
      return getComment(ALBUM);
   }

   public String[] getAlbums() {
      return getComments(ALBUM);
   }

   public String getTrackNumber() {
      return getComment(TRACKNUMBER);
   }

   public String[] getTrackNumbers() {
      return getComments(TRACKNUMBER);
   }

   public String getArtist() {
      return getComment(ARTIST);
   }

   public String[] getArtists() {
      return getComments(ARTIST);
   }

   public String getPerformer() {
      return getComment(PERFORMER);
   }

   public String[] getPerformers() {
      return getComments(PERFORMER);
   }

   public String getCopyright() {
      return getComment(COPYRIGHT);
   }

   public String[] getCopyrights() {
      return getComments(COPYRIGHT);
   }

   public String getLicense() {
      return getComment(LICENSE);
   }

   public String[] getLicenses() {
      return getComments(LICENSE);
   }

   public String getOrganization() {
      return getComment(ORGANIZATION);
   }

   public String[] getOrganizations() {
      return getComments(ORGANIZATION);
   }

   public String getDescription() {
      return getComment(DESCRIPTION);
   }

   public String[] getDescriptions() {
      return getComments(DESCRIPTION);
   }

   public String getGenre() {
      return getComment(GENRE);
   }

   public String[] getGenres() {
      return getComments(GENRE);
   }

   public String getDate() {
      return getComment(DATE);
   }

   public String[] getDates() {
      return getComments(DATE);
   }

   public String getLocation() {
      return getComment(LOCATION);
   }

   public String[] getLocations() {
      return getComments(LOCATION);
   }

   public String getContact() {
      return getComment(CONTACT);
   }

   public String[] getContacts() {
      return getComments(CONTACT);
   }

   public String getIsrc() {
      return getComment(ISRC);
   }

   public String[] getIsrcs() {
      return getComments(ISRC);
   }


   private String getString(BitInputStream source) throws IOException, VorbisFormatException {

      int length=source.getInt(32);

      byte[] strArray=new byte[length];

      for(int i=0; i<length; i++) {
         strArray[i]=(byte)source.getInt(8);
      }

      return new String(strArray, "UTF-8");
   }

}