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

package com.jcraft.jorbis;

class PsyLook {
  int n;
  PsyInfo vi;

  float[][][] tonecurves;
  float[][] peakatt;
  float[][][] noisecurves;

  float[] ath;
  int[] octave;

  void init(PsyInfo vi, int n, int rate){
    /*
    float rate2=rate/2.;
    //memset(p,0,sizeof(vorbis_look_psy));
    ath=new float[n];
    octave=new int[n];
    this.vi=vi;
    this.n=n;

    // set up the lookups for a given blocksize and sample rate
    // Vorbis max sample rate is limited by 26 Bark (54kHz)
    set_curve(ATH_Bark_dB, ath,n,rate);
    for(int i=0;i<n;i++)
      ath[i]=fromdB(ath[i]+vi.ath_att);

    for(int i=0;i<n;i++){
      int oc=rint(toOC((i+.5)*rate2/n)*2.);
      if(oc<0)oc=0;
      if(oc>12)oc=12;
      octave[i]=oc;
    }  

    tonecurves=malloc(13*sizeof(float **));
    noisecurves=malloc(13*sizeof(float **));
    peakatt=malloc(7*sizeof(float *));
    for(int i=0;i<13;i++){
      tonecurves[i]=malloc(9*sizeof(float *));
      noisecurves[i]=malloc(9*sizeof(float *));
    }
    for(i=0;i<7;i++)
      peakatt[i]=malloc(5*sizeof(float));

    for(i=0;i<13;i++){
      for(j=0;j<9;j++){
	tonecurves[i][j]=malloc(EHMER_MAX*sizeof(float));
	noisecurves[i][j]=malloc(EHMER_MAX*sizeof(float));
      }
    }

    // OK, yeah, this was a silly way to do it
    memcpy(tonecurves[0][2],tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[0][4],tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[0][6],tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[0][8],tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(tonecurves[2][2],tone_250_40dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[2][4],tone_250_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[2][6],tone_250_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[2][8],tone_250_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(tonecurves[4][2],tone_500_40dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[4][4],tone_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[4][6],tone_500_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[4][8],tone_500_100dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(tonecurves[6][2],tone_1000_40dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[6][4],tone_1000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[6][6],tone_1000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[6][8],tone_1000_100dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(tonecurves[8][2],tone_2000_40dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[8][4],tone_2000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[8][6],tone_2000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[8][8],tone_2000_100dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(tonecurves[10][2],tone_4000_40dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[10][4],tone_4000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[10][6],tone_4000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[10][8],tone_4000_100dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(tonecurves[12][2],tone_4000_40dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[12][4],tone_4000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[12][6],tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(tonecurves[12][8],tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);


    memcpy(noisecurves[0][2],noise_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[0][4],noise_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[0][6],noise_500_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[0][8],noise_500_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(noisecurves[2][2],noise_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[2][4],noise_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[2][6],noise_500_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[2][8],noise_500_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(noisecurves[4][2],noise_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[4][4],noise_500_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[4][6],noise_500_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[4][8],noise_500_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(noisecurves[6][2],noise_1000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[6][4],noise_1000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[6][6],noise_1000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[6][8],noise_1000_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(noisecurves[8][2],noise_2000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[8][4],noise_2000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[8][6],noise_2000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[8][8],noise_2000_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(noisecurves[10][2],noise_4000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[10][4],noise_4000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[10][6],noise_4000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[10][8],noise_4000_80dB_SL,sizeof(float)*EHMER_MAX);

    memcpy(noisecurves[12][2],noise_4000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[12][4],noise_4000_60dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[12][6],noise_4000_80dB_SL,sizeof(float)*EHMER_MAX);
    memcpy(noisecurves[12][8],noise_4000_80dB_SL,sizeof(float)*EHMER_MAX);

    setup_curve(tonecurves[0],0,vi.toneatt_125Hz);
    setup_curve(tonecurves[2],2,vi.toneatt_250Hz);
    setup_curve(tonecurves[4],4,vi.toneatt_500Hz);
    setup_curve(tonecurves[6],6,vi.toneatt_1000Hz);
    setup_curve(tonecurves[8],8,vi.toneatt_2000Hz);
    setup_curve(tonecurves[10],10,vi.toneatt_4000Hz);
    setup_curve(tonecurves[12],12,vi.toneatt_8000Hz);
    
    setup_curve(noisecurves[0],0,vi.noiseatt_125Hz);
    setup_curve(noisecurves[2],2,vi.noiseatt_250Hz);
    setup_curve(noisecurves[4],4,vi.noiseatt_500Hz);
    setup_curve(noisecurves[6],6,vi.noiseatt_1000Hz);
    setup_curve(noisecurves[8],8,vi.noiseatt_2000Hz);
    setup_curve(noisecurves[10],10,vi.noiseatt_4000Hz);
    setup_curve(noisecurves[12],12,vi.noiseatt_8000Hz);

    for(i=1;i<13;i+=2){
      for(j=0;j<9;j++){
	interp_curve_dB(tonecurves[i][j],
			tonecurves[i-1][j],
			tonecurves[i+1][j],.5);
	interp_curve_dB(noisecurves[i][j],
			noisecurves[i-1][j],
			noisecurves[i+1][j],.5);
      }
    }
    for(i=0;i<5;i++){
      peakatt[0][i]=fromdB(vi.peakatt_125Hz[i]);
      peakatt[1][i]=fromdB(vi.peakatt_250Hz[i]);
      peakatt[2][i]=fromdB(vi.peakatt_500Hz[i]);
      peakatt[3][i]=fromdB(vi.peakatt_1000Hz[i]);
      peakatt[4][i]=fromdB(vi.peakatt_2000Hz[i]);
      peakatt[5][i]=fromdB(vi.peakatt_4000Hz[i]);
      peakatt[6][i]=fromdB(vi.peakatt_8000Hz[i]);
    }
  */
  }
}
