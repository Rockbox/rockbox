package de.jarnbjo.util.audio;

import java.io.*;
import javax.sound.sampled.*;

public class FadeableAudioInputStream extends AudioInputStream {

   private AudioInputStream stream;
   private boolean fading=false;
   private double phi=0.0;

   public FadeableAudioInputStream(AudioInputStream stream) throws IOException {
      super(stream, stream.getFormat(), -1L);
   }

   public void fadeOut() {
      fading=true;
      phi=0.0;
   }

   public int read(byte[] b) throws IOException {
      return read(b, 0, b.length);
   }

   public int read(byte[] b, int offset, int length) throws IOException {
      int read=super.read(b, offset, length);

      //System.out.println("read "+read);

      if(fading) {
         int j=0, l=0, r=0;
         double gain=0.0;

         for(int i=offset; i<offset+read; i+=4) {
            j=i;
            l=((int)b[j++])&0xff;
            l|=((int)b[j++])<<8;
            r=((int)b[j++])&0xff;
            r|=((int)b[j])<<8;

            if(phi<Math.PI/2) {
               phi+=0.000015;
            }

            gain=Math.cos(phi);
            //System.out.println("gain "+gain);

            l=(int)(l*gain);
            r=(int)(r*gain);

            j=i;
            b[j++]=(byte)(l&0xff);
            b[j++]=(byte)((l>>8)&0xff);
            b[j++]=(byte)(r&0xff);
            b[j++]=(byte)((r>>8)&0xff);
         }
      }

      return read;
   }

}