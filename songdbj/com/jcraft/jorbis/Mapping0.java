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

import com.jcraft.jogg.*;

class Mapping0 extends FuncMapping{
  static int seq=0;
  void free_info(Object imap){};
  void free_look(Object imap){
/*
    LookMapping0 l=(LookMapping0)imap;
    InfoMapping0 info=l.map;
    if(l!=null){
      for(int i=0;i<l.map.submaps;i++){
        l.time_func[i].free_look(l.time_look[i]);
        l.floor_func[i].free_look(l.floor_look[i]);
        l.residue_func[i].free_look(l.residue_look[i]);
        if(l.psy_look!=null)l.psy_look[i].clear();
      }
    }

    if(l.floor_state!=null){
      for(int i=0;i<l.ch;i++)
  	l.floor_func[info.chmuxlist[i]].free_state(l.floor_state[i]);
      //free(l.floor_state);
    }

    if(l.decay!=null){
      for(int i=0;i<l.ch;i++){
	//if(l.decay[i])free(l->decay[i]);
        l.decay[i]=null;
      }
      //free(l->decay);
      l.decay=null;
    }
    //free(l->time_func);
    //free(l->floor_func);
    //free(l->residue_func);
    //free(l->time_look);
    //free(l->floor_look);
    //free(l->residue_look);
    //f(l->psy_look)free(l->psy_look);
    l.time_func=null;
    l.floor_func=null;
    l.residue_func=null;
    l.time_look=null;
    l.floor_look=null;
    l.residue_look=null;
    //memset(l,0,sizeof(vorbis_look_mapping0));
    //free(l);
*/
  }

  Object look(DspState vd, InfoMode vm, Object m){
//System.err.println("Mapping0.look");
    Info vi=vd.vi;
    LookMapping0 look=new LookMapping0();
    InfoMapping0 info=look.map=(InfoMapping0)m;
    look.mode=vm;
  
    look.time_look=new Object[info.submaps];
    look.floor_look=new Object[info.submaps];
    look.residue_look=new Object[info.submaps];

/*
    if(vd.analysisp!=0){
      look.floor_state=new Object[vi.channels];
    }
    if(vi.psys!=0){
      look.psy_look=new PsyLook[info.submaps];
      for(int i=0; i<info.submaps; i++){ look.psy_look[i]=new PsyLook(); }
    }
*/

    look.time_func=new FuncTime[info.submaps];
    look.floor_func=new FuncFloor[info.submaps];
    look.residue_func=new FuncResidue[info.submaps];
  
    for(int i=0;i<info.submaps;i++){
      int timenum=info.timesubmap[i];
      int floornum=info.floorsubmap[i];
      int resnum=info.residuesubmap[i];

      look.time_func[i]=FuncTime.time_P[vi.time_type[timenum]];
      look.time_look[i]=look.time_func[i].look(vd,vm,vi.time_param[timenum]);
      look.floor_func[i]=FuncFloor.floor_P[vi.floor_type[floornum]];
      look.floor_look[i]=look.floor_func[i].
                         look(vd,vm,vi.floor_param[floornum]);
      look.residue_func[i]=FuncResidue.residue_P[vi.residue_type[resnum]];
      look.residue_look[i]=look.residue_func[i].
                           look(vd,vm,vi.residue_param[resnum]);

/*    
      if(vi.psys!=0 && vd.analysisp!=0){
        int psynum=info.psysubmap[i];
	look.psy_look[i].init(vi.psy_param[psynum],
			      vi.blocksizes[vm.blockflag]/2,vi.rate);
      }
*/
    }

    if(vi.psys!=0 && vd.analysisp!=0){
	/*
    if(info->psy[0] != info->psy[1]){

      int psynum=info->psy[0];
      look->psy_look[0]=_ogg_calloc(1,sizeof(vorbis_look_psy));      
      _vp_psy_init(look->psy_look[0],ci->psy_param[psynum],
		   ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);

      psynum=info->psy[1];
      look->psy_look[1]=_ogg_calloc(1,sizeof(vorbis_look_psy));      
      _vp_psy_init(look->psy_look[1],ci->psy_param[psynum],
		   ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);
    }else{

      int psynum=info->psy[0];
      look->psy_look[0]=_ogg_calloc(1,sizeof(vorbis_look_psy));      
      look->psy_look[1]=look->psy_look[0];
     _vp_psy_init(look->psy_look[0],ci->psy_param[psynum],
		   ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);

    }
	*/
    }

    look.ch=vi.channels;
//  if(vd->analysisp)drft_init(&look->fft_look,ci->blocksizes[vm->blockflag]);

    return(look);
//return null;
  }

  void pack(Info vi, Object imap, Buffer opb){
    InfoMapping0 info=(InfoMapping0)imap;

  /* another 'we meant to do it this way' hack...  up to beta 4, we
     packed 4 binary zeros here to signify one submapping in use.  We
     now redefine that to mean four bitflags that indicate use of
     deeper features; bit0:submappings, bit1:coupling,
     bit2,3:reserved. This is backward compatable with all actual uses
     of the beta code. */

    if(info.submaps>1){
      opb.write(1,1);
      opb.write(info.submaps-1,4);
    }
    else{
      opb.write(0,1);
    }

    if(info.coupling_steps>0){
      opb.write(1,1);
      opb.write(info.coupling_steps-1,8);
      for(int i=0;i<info.coupling_steps;i++){
        opb.write(info.coupling_mag[i],ilog2(vi.channels));
        opb.write(info.coupling_ang[i],ilog2(vi.channels));
      }
    }
    else{
      opb.write(0,1);
    }
  
    opb.write(0,2); /* 2,3:reserved */

    /* we don't write the channel submappings if we only have one... */
    if(info.submaps>1){
      for(int i=0;i<vi.channels;i++)
        opb.write(info.chmuxlist[i],4);
    }
    for(int i=0;i<info.submaps;i++){
      opb.write(info.timesubmap[i],8);
      opb.write(info.floorsubmap[i],8);
      opb.write(info.residuesubmap[i],8);
    }
  }

  // also responsible for range checking
  Object unpack(Info vi, Buffer opb){
    InfoMapping0 info=new InfoMapping0();

    // !!!!
    if(opb.read(1)!=0){
      info.submaps=opb.read(4)+1;
    }
    else{
      info.submaps=1;
    }

    if(opb.read(1)!=0){
      info.coupling_steps=opb.read(8)+1;

      for(int i=0;i<info.coupling_steps;i++){
        int testM=info.coupling_mag[i]=opb.read(ilog2(vi.channels));
        int testA=info.coupling_ang[i]=opb.read(ilog2(vi.channels));

        if(testM<0 ||
           testA<0 ||
           testM==testA ||
           testM>=vi.channels ||
           testA>=vi.channels){
	  //goto err_out;
          info.free();
          return(null);
        }
      }
    }

    if(opb.read(2)>0){ /* 2,3:reserved */
      //goto err_out;
      info.free();
      return(null);
    }

    if(info.submaps>1){
      for(int i=0;i<vi.channels;i++){
        info.chmuxlist[i]=opb.read(4);
        if(info.chmuxlist[i]>=info.submaps){
	  //goto err_out;
          info.free();
          return(null);
	}
      }
    }

    for(int i=0;i<info.submaps;i++){
      info.timesubmap[i]=opb.read(8);
      if(info.timesubmap[i]>=vi.times){
        //goto err_out;
        info.free();
        return(null);
      }
      info.floorsubmap[i]=opb.read(8);
      if(info.floorsubmap[i]>=vi.floors){
	//goto err_out;
        info.free();
        return(null);
      }
      info.residuesubmap[i]=opb.read(8);
      if(info.residuesubmap[i]>=vi.residues){
	//goto err_out;
        info.free();
        return(null);
      }
    }
    return info;
    //err_out:
    //free_info(info);
    //return(NULL);
  }

/*
  // no time mapping implementation for now 
  static int seq=0;
  int forward(Block vb, Object l){
    DspState vd=vb.vd;
    Info vi=vd.vi;
    LookMapping0 look=(LookMapping0)l;
    InfoMapping0 info=look.map;
    InfoMode mode=look.mode;
    int n=vb.pcmend;
    float[] window=vd.window[vb.W][vb.lW][vb.nW][mode.windowtype];

    float[][] pcmbundle=new float[vi.channles][];
    int[] nonzero=new int[vi.channels];
 
     // time domain pre-window: NONE IMPLEMENTED

     // window the PCM data: takes PCM vector, vb; modifies PCM vector

     for(int i=0;i<vi.channels;i++){
       float[] pcm=vb.pcm[i];
       for(int j=0;j<n;j++)
         pcm[j]*=window[j];
     }
	    
     // time-domain post-window: NONE IMPLEMENTED

     // transform the PCM data; takes PCM vector, vb; modifies PCM vector
     // only MDCT right now....
     for(int i=0;i<vi.channels;i++){
       float[] pcm=vb.pcm[i];
       mdct_forward(vd.transform[vb.W][0],pcm,pcm);
     }

     {
       float[] floor=_vorbis_block_alloc(vb,n*sizeof(float)/2);
    
       for(int i=0;i<vi.channels;i++){
         float[] pcm=vb.pcm[i];
         float[] decay=look.decay[i];
         int submap=info.chmuxlist[i];

         // if some other mode/mapping was called last frame, our decay
         // accumulator is out of date.  Clear it.
         //if(look.lastframe+1 != vb->sequence)
  	 //  memset(decay,0,n*sizeof(float)/2);

         // perform psychoacoustics; do masking
         _vp_compute_mask(look.psy_look[submap],pcm,floor,decay);
 
         _analysis_output("mdct",seq,pcm,n/2,0,1);
         _analysis_output("lmdct",seq,pcm,n/2,0,0);
         _analysis_output("prefloor",seq,floor,n/2,0,1);

         // perform floor encoding
         nonzero[i]=look.floor_func[submap].
 	            forward(vb,look.floor_look[submap],floor,floor,look.floor_state[i]);

         _analysis_output("floor",seq,floor,n/2,0,1);

         // apply the floor, do optional noise levelling
         _vp_apply_floor(look->psy_look+submap,pcm,floor);
      
         _analysis_output("res",seq++,pcm,n/2,0,0);
     }
    
     // perform residue encoding with residue mapping; this is
     // multiplexed.  All the channels belonging to one submap are
     // encoded (values interleaved), then the next submap, etc
    
     for(int i=0;i<info.submaps;i++){
       int ch_in_bundle=0;
        for(int j=0;j<vi.channels;j++){
	  if(info.chmuxlist[j]==i && nonzero[j]==1){
	    pcmbundle[ch_in_bundle++]=vb.pcm[j];
 	  }
        }
        look.residue_func[i].forward(vb,look.residue_look[i], pcmbundle,ch_in_bundle);
      }
    }
    look.lastframe=vb.sequence;
    return(0);
  }
*/

  float[][] pcmbundle=null;
  int[] zerobundle=null;
  int[] nonzero=null;
  Object[] floormemo=null;

  synchronized int inverse(Block vb, Object l){
    //System.err.println("Mapping0.inverse");
    DspState vd=vb.vd;
    Info vi=vd.vi;
    LookMapping0 look=(LookMapping0)l;
    InfoMapping0 info=look.map;
    InfoMode mode=look.mode;
    int n=vb.pcmend=vi.blocksizes[vb.W];

    float[] window=vd.window[vb.W][vb.lW][vb.nW][mode.windowtype];
    // float[][] pcmbundle=new float[vi.channels][];
    // int[] nonzero=new int[vi.channels];
    if(pcmbundle==null || pcmbundle.length<vi.channels){
      pcmbundle=new float[vi.channels][];
      nonzero=new int[vi.channels];
      zerobundle=new int[vi.channels];
      floormemo=new Object[vi.channels];
    }
  
    // time domain information decode (note that applying the
    // information would have to happen later; we'll probably add a
    // function entry to the harness for that later
    // NOT IMPLEMENTED

    // recover the spectral envelope; store it in the PCM vector for now 
    for(int i=0;i<vi.channels;i++){
      float[] pcm=vb.pcm[i];
      int submap=info.chmuxlist[i];

      floormemo[i]=look.floor_func[submap].inverse1(vb,look.
						    floor_look[submap],
						    floormemo[i]
						    );
      if(floormemo[i]!=null){ nonzero[i]=1; }
      else{ nonzero[i]=0; }
      for(int j=0; j<n/2; j++){
        pcm[j]=0;
      }                 

      //_analysis_output("ifloor",seq+i,pcm,n/2,0,1);
    }

    for(int i=0; i<info.coupling_steps; i++){
      if(nonzero[info.coupling_mag[i]]!=0 ||
         nonzero[info.coupling_ang[i]]!=0){
        nonzero[info.coupling_mag[i]]=1;
        nonzero[info.coupling_ang[i]]=1;
      }
    }

    // recover the residue, apply directly to the spectral envelope

    for(int i=0;i<info.submaps;i++){
      int ch_in_bundle=0;
      for(int j=0;j<vi.channels;j++){
        if(info.chmuxlist[j]==i){
          if(nonzero[j]!=0){
            zerobundle[ch_in_bundle]=1;
	  }
          else{
            zerobundle[ch_in_bundle]=0;
	  }
	  pcmbundle[ch_in_bundle++]=vb.pcm[j];
	}
      }

      look.residue_func[i].inverse(vb,look.residue_look[i],
				   pcmbundle,zerobundle,ch_in_bundle);
    }


    for(int i=info.coupling_steps-1;i>=0;i--){
      float[] pcmM=vb.pcm[info.coupling_mag[i]];
      float[] pcmA=vb.pcm[info.coupling_ang[i]];

      for(int j=0;j<n/2;j++){
        float mag=pcmM[j];
        float ang=pcmA[j];

        if(mag>0){
          if(ang>0){
            pcmM[j]=mag;
            pcmA[j]=mag-ang;
          }
          else{
            pcmA[j]=mag;
            pcmM[j]=mag+ang;
          }
	}
        else{
          if(ang>0){
            pcmM[j]=mag;
            pcmA[j]=mag+ang;
          }
          else{
            pcmA[j]=mag;
            pcmM[j]=mag-ang;
          }
	}
      }
    }

//    /* compute and apply spectral envelope */

    for(int i=0;i<vi.channels;i++){
      float[] pcm=vb.pcm[i];
      int submap=info.chmuxlist[i];
      look.floor_func[submap].inverse2(vb,look.floor_look[submap],floormemo[i],pcm);
    }

    // transform the PCM data; takes PCM vector, vb; modifies PCM vector
    // only MDCT right now....

    for(int i=0;i<vi.channels;i++){
      float[] pcm=vb.pcm[i];
      //_analysis_output("out",seq+i,pcm,n/2,0,0);
      ((Mdct)vd.transform[vb.W][0]).backward(pcm,pcm);
    }

    // now apply the decoded pre-window time information
    // NOT IMPLEMENTED
  
    // window the data
    for(int i=0;i<vi.channels;i++){
      float[] pcm=vb.pcm[i];
      if(nonzero[i]!=0){
        for(int j=0;j<n;j++){
	  pcm[j]*=window[j];
        }
      }
      else{
        for(int j=0;j<n;j++){
	  pcm[j]=0.f;
	}
      }
      //_analysis_output("final",seq++,pcm,n,0,0);
    }
	    
    // now apply the decoded post-window time information
    // NOT IMPLEMENTED
    // all done!
    return(0);
  }


  private static int ilog2(int v){
    int ret=0;
    while(v>1){
      ret++;
      v>>>=1;
    }
    return(ret);
  }
}

class InfoMapping0{
  int   submaps;  // <= 16
  int[] chmuxlist=new int[256];   // up to 256 channels in a Vorbis stream
  
  int[] timesubmap=new int[16];   // [mux]
  int[] floorsubmap=new int[16];  // [mux] submap to floors
  int[] residuesubmap=new int[16];// [mux] submap to residue
  int[] psysubmap=new int[16];    // [mux]; encode only

  int   coupling_steps;
  int[] coupling_mag=new int[256];
  int[] coupling_ang=new int[256];

  void free(){
    chmuxlist=null;
    timesubmap=null;
    floorsubmap=null;
    residuesubmap=null;
    psysubmap=null;

    coupling_mag=null;
    coupling_ang=null;
  }
}

class LookMapping0{
  InfoMode mode;
  InfoMapping0 map;
  Object[] time_look;
  Object[] floor_look;
  Object[] floor_state;
  Object[] residue_look; 
  PsyLook[] psy_look;

  FuncTime[] time_func; 
  FuncFloor[] floor_func; 
  FuncResidue[] residue_func;

  int ch;
  float[][] decay;
  int lastframe; // if a different mode is called, we need to 
                 // invalidate decay and floor state
}
