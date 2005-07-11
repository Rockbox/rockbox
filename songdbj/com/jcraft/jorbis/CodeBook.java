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

class CodeBook{
  int dim;            // codebook dimensions (elements per vector)
  int entries;        // codebook entries
  StaticCodeBook c=new StaticCodeBook();

  float[] valuelist; // list of dim*entries actual entry values
  int[] codelist;     // list of bitstream codewords for each entry
  DecodeAux decode_tree;

  // returns the number of bits
  int encode(int a, Buffer b){
    b.write(codelist[a], c.lengthlist[a]);
    return(c.lengthlist[a]);
  }

  // One the encode side, our vector writers are each designed for a
  // specific purpose, and the encoder is not flexible without modification:
  // 
  // The LSP vector coder uses a single stage nearest-match with no
  // interleave, so no step and no error return.  This is specced by floor0
  // and doesn't change.
  // 
  // Residue0 encoding interleaves, uses multiple stages, and each stage
  // peels of a specific amount of resolution from a lattice (thus we want
  // to match by threshhold, not nearest match).  Residue doesn't *have* to
  // be encoded that way, but to change it, one will need to add more
  // infrastructure on the encode side (decode side is specced and simpler)

  // floor0 LSP (single stage, non interleaved, nearest match)
  // returns entry number and *modifies a* to the quantization value
  int errorv(float[] a){
    int best=best(a,1);
    for(int k=0;k<dim;k++){
      a[k]=valuelist[best*dim+k];
    }
    return(best);
  }

  // returns the number of bits and *modifies a* to the quantization value
  int encodev(int best, float[] a, Buffer b){
    for(int k=0;k<dim;k++){
      a[k]=valuelist[best*dim+k];
    }
    return(encode(best,b));
  }

  // res0 (multistage, interleave, lattice)
  // returns the number of bits and *modifies a* to the remainder value
  int encodevs(float[] a, Buffer b, int step,int addmul){
    int best=besterror(a,step,addmul);
    return(encode(best,b));
  }

  private int[] t=new int[15];  // decodevs_add is synchronized for re-using t.
  synchronized int decodevs_add(float[]a, int offset, Buffer b, int n){
    int step=n/dim;
    int entry;
    int i,j,o;

    if(t.length<step){
      t=new int[step];
    }

    for(i = 0; i < step; i++){
      entry=decode(b);
      if(entry==-1)return(-1);
      t[i]=entry*dim;
    }
    for(i=0,o=0;i<dim;i++,o+=step){
      for(j=0;j<step;j++){
	a[offset+o+j]+=valuelist[t[j]+i];
      }
    }

    return(0);
  }

  int decodev_add(float[]a, int offset, Buffer b,int n){
    int i,j,entry;
    int t;

    if(dim>8){
      for(i=0;i<n;){
        entry = decode(b);
        if(entry==-1)return(-1);
	t=entry*dim;
	for(j=0;j<dim;){
	  a[offset+(i++)]+=valuelist[t+(j++)];
	}
      }
    }
    else{
      for(i=0;i<n;){
	entry=decode(b);
	if(entry==-1)return(-1);
	t=entry*dim;
	j=0;
	switch(dim){
	case 8:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 7:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 6:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 5:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 4:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 3:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 2:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 1:
	  a[offset+(i++)]+=valuelist[t+(j++)];
	case 0:
	break;
	}
      }
    }    
    return(0);
  }

  int decodev_set(float[] a,int offset, Buffer b, int n){
    int i,j,entry;
    int t;

    for(i=0;i<n;){
      entry = decode(b);
      if(entry==-1)return(-1);
      t=entry*dim;
      for(j=0;j<dim;){
        a[offset+i++]=valuelist[t+(j++)];
      }
    }
    return(0);
  }

  int decodevv_add(float[][] a, int offset,int ch, Buffer b,int n){
    int i,j,k,entry;
    int chptr=0;
    //System.out.println("decodevv_add: a="+a+",b="+b+",valuelist="+valuelist);

    for(i=offset/ch;i<(offset+n)/ch;){
      entry = decode(b);
      if(entry==-1)return(-1);

      int t = entry*dim;
      for(j=0;j<dim;j++){
        a[chptr++][i]+=valuelist[t+j];
        if(chptr==ch){
          chptr=0;
	  i++;
	}
      }
    }
    return(0);
  }


  // Decode side is specced and easier, because we don't need to find
  // matches using different criteria; we simply read and map.  There are
  // two things we need to do 'depending':
  //   
  // We may need to support interleave.  We don't really, but it's
  // convenient to do it here rather than rebuild the vector later.
  //
  // Cascades may be additive or multiplicitive; this is not inherent in
  // the codebook, but set in the code using the codebook.  Like
  // interleaving, it's easiest to do it here.  
  // stage==0 -> declarative (set the value)
  // stage==1 -> additive
  // stage==2 -> multiplicitive

  // returns the entry number or -1 on eof
  int decode(Buffer b){
    int ptr=0;
    DecodeAux t=decode_tree;
    int lok=b.look(t.tabn);
    //System.err.println(this+" "+t+" lok="+lok+", tabn="+t.tabn);

    if(lok>=0){
      ptr=t.tab[lok];
      b.adv(t.tabl[lok]);
      if(ptr<=0){
        return -ptr;
      }
    }
    do{
      switch(b.read1()){
      case 0:
	ptr=t.ptr0[ptr];
	break;
      case 1:
	ptr=t.ptr1[ptr];
	break;
      case -1:
      default:
	return(-1);
      }
    }
    while(ptr>0);
    return(-ptr);
  }

  // returns the entry number or -1 on eof
  int decodevs(float[] a, int index, Buffer b, int step,int addmul){
    int entry=decode(b);
    if(entry==-1)return(-1);
    switch(addmul){
    case -1:
      for(int i=0,o=0;i<dim;i++,o+=step)
	a[index+o]=valuelist[entry*dim+i];
      break;
    case 0:
      for(int i=0,o=0;i<dim;i++,o+=step)
	a[index+o]+=valuelist[entry*dim+i];
      break;
    case 1:
      for(int i=0,o=0;i<dim;i++,o+=step)
	a[index+o]*=valuelist[entry*dim+i];
      break;
    default:
      //System.err.println("CodeBook.decodeves: addmul="+addmul); 
    }
    return(entry);
  }

  int best(float[] a, int step){
    EncodeAuxNearestMatch nt=c.nearest_tree;
    EncodeAuxThreshMatch tt=c.thresh_tree;
    int ptr=0;

    // we assume for now that a thresh tree is the only other possibility
    if(tt!=null){
      int index=0;
      // find the quant val of each scalar
      for(int k=0,o=step*(dim-1);k<dim;k++,o-=step){
	int i;
	// linear search the quant list for now; it's small and although
	// with > 8 entries, it would be faster to bisect, this would be
	// a misplaced optimization for now
	for(i=0;i<tt.threshvals-1;i++){
	  if(a[o]<tt.quantthresh[i]){
	    break;
	  }
	}
	index=(index*tt.quantvals)+tt.quantmap[i];
      }
      // regular lattices are easy :-)
      if(c.lengthlist[index]>0){
	// is this unused?  If so, we'll
	// use a decision tree after all
	// and fall through
	return(index);
      }
    }
    if(nt!=null){
      // optimized using the decision tree
      while(true){
	float c=0.f;
	int p=nt.p[ptr];
	int q=nt.q[ptr];
	for(int k=0,o=0;k<dim;k++,o+=step){
	  c+=(valuelist[p+k]-valuelist[q+k])*
	     (a[o]-(valuelist[p+k]+valuelist[q+k])*.5);
	}
	if(c>0.){ // in A
	  ptr= -nt.ptr0[ptr];
	}
	else{     // in B
	  ptr= -nt.ptr1[ptr];
	}
	if(ptr<=0)break;
      }
      return(-ptr);
    }

    // brute force it!
    {
      int besti=-1;
      float best=0.f;
      int e=0;
      for(int i=0;i<entries;i++){
	if(c.lengthlist[i]>0){
	  float _this=dist(dim, valuelist, e, a, step);
	  if(besti==-1 || _this<best){
	    best=_this;
	    besti=i;
	  }
	}
	e+=dim;
      }
      return(besti);
    }
  }

  // returns the entry number and *modifies a* to the remainder value
  int besterror(float[] a, int step, int addmul){
    int best=best(a,step);
    switch(addmul){
    case 0:
      for(int i=0,o=0;i<dim;i++,o+=step)
	a[o]-=valuelist[best*dim+i];
      break;
    case 1:
      for(int i=0,o=0;i<dim;i++,o+=step){
	float val=valuelist[best*dim+i];
	if(val==0){
	  a[o]=0;
	}else{
	  a[o]/=val;
	}
      }
      break;
    }
    return(best);
  }

  void clear(){
    // static book is not cleared; we're likely called on the lookup and
    // the static codebook belongs to the info struct
    //if(decode_tree!=null){
    //  free(b->decode_tree->ptr0);
    //  free(b->decode_tree->ptr1);
    //  memset(b->decode_tree,0,sizeof(decode_aux));
    //  free(b->decode_tree);
    //}
    //if(valuelist!=null)free(b->valuelist);
    //if(codelist!=null)free(b->codelist);
    //memset(b,0,sizeof(codebook));
  }

  private static float dist(int el, float[] ref, int index, float[] b, int step){
    float acc=(float)0.;
    for(int i=0; i<el; i++){
      float val=(ref[index+i]-b[i*step]);
      acc+=val*val;
    }
    return(acc);
  }

/*
  int init_encode(StaticCodeBook s){
    //memset(c,0,sizeof(codebook));
    c=s;
    entries=s.entries;
    dim=s.dim;
    codelist=make_words(s.lengthlist, s.entries);
    valuelist=s.unquantize();
    return(0);
  }
*/

  int init_decode(StaticCodeBook s){
    //memset(c,0,sizeof(codebook));
    c=s;
    entries=s.entries;
    dim=s.dim;
    valuelist=s.unquantize();

    decode_tree=make_decode_tree();
    if(decode_tree==null){
      //goto err_out;
      clear();
      return(-1);
    }
    return(0);
//  err_out:
//    vorbis_book_clear(c);
//    return(-1);
  }

  // given a list of word lengths, generate a list of codewords.  Works
  // for length ordered or unordered, always assigns the lowest valued
  // codewords first.  Extended to handle unused entries (length 0)
  static int[] make_words(int[] l, int n){
    int[] marker=new int[33];
    int[] r=new int[n];
    //memset(marker,0,sizeof(marker));

    for(int i=0;i<n;i++){
      int length=l[i];
      if(length>0){
	int entry=marker[length];
      
	// when we claim a node for an entry, we also claim the nodes
	// below it (pruning off the imagined tree that may have dangled
	// from it) as well as blocking the use of any nodes directly
	// above for leaves
      
	// update ourself
	if(length<32 && (entry>>>length)!=0){
	  // error condition; the lengths must specify an overpopulated tree
	  //free(r);
	  return(null);
	}
	r[i]=entry;
    
	// Look to see if the next shorter marker points to the node
	// above. if so, update it and repeat.
	{
	  for(int j=length;j>0;j--){
	    if((marker[j]&1)!=0){
	      // have to jump branches
	      if(j==1)marker[1]++;
	      else marker[j]=marker[j-1]<<1;
	      break; // invariant says next upper marker would already
                     // have been moved if it was on the same path
	    }
	    marker[j]++;
	  }
	}
      
	// prune the tree; the implicit invariant says all the longer
	// markers were dangling from our just-taken node.  Dangle them
	// from our *new* node.
	for(int j=length+1;j<33;j++){
	  if((marker[j]>>>1) == entry){
	    entry=marker[j];
	    marker[j]=marker[j-1]<<1;
	  }
	  else{
	    break;
	  }
	}    
      }
    }

    // bitreverse the words because our bitwise packer/unpacker is LSb
    // endian
    for(int i=0;i<n;i++){
      int temp=0;
      for(int j=0;j<l[i];j++){
	temp<<=1;
	temp|=(r[i]>>>j)&1;
      }
      r[i]=temp;
    }

    return(r);
  }

  // build the decode helper tree from the codewords 
  DecodeAux make_decode_tree(){
    int top=0;
    DecodeAux t=new DecodeAux();
    int[] ptr0=t.ptr0=new int[entries*2];
    int[] ptr1=t.ptr1=new int[entries*2];
    int[] codelist=make_words(c.lengthlist, c.entries);

    if(codelist==null)return(null);
    t.aux=entries*2;

    for(int i=0;i<entries;i++){
      if(c.lengthlist[i]>0){
	int ptr=0;
	int j;
	for(j=0;j<c.lengthlist[i]-1;j++){
	  int bit=(codelist[i]>>>j)&1;
	  if(bit==0){
	    if(ptr0[ptr]==0){
	      ptr0[ptr]=++top;
	    }
	    ptr=ptr0[ptr];
	  }
	  else{
	    if(ptr1[ptr]==0){
	      ptr1[ptr]= ++top;
	    }
	    ptr=ptr1[ptr];
	  }
	}

	if(((codelist[i]>>>j)&1)==0){ ptr0[ptr]=-i; }
	else{ ptr1[ptr]=-i; }

      }
    }
    //free(codelist);

    t.tabn = ilog(entries)-4;

    if(t.tabn<5)t.tabn=5;
    int n = 1<<t.tabn;
    t.tab = new int[n];
    t.tabl = new int[n];
    for(int i = 0; i < n; i++){
      int p = 0;
      int j=0;
      for(j = 0; j < t.tabn && (p > 0 || j == 0); j++){
        if ((i&(1<<j))!=0){
	  p = ptr1[p];
	}
        else{
	  p = ptr0[p];
	}
      }
      t.tab[i]=p;  // -code
      t.tabl[i]=j; // length 
    }

    return(t);
  }

  private static int ilog(int v){
    int ret=0;
    while(v!=0){
      ret++;
      v>>>=1;
    }
    return(ret);
  }

/*
  // TEST
  // Simple enough; pack a few candidate codebooks, unpack them.  Code a
  // number of vectors through (keeping track of the quantized values),
  // and decode using the unpacked book.  quantized version of in should
  // exactly equal out

  //#include "vorbis/book/lsp20_0.vqh"
  //#include "vorbis/book/lsp32_0.vqh"
  //#include "vorbis/book/res0_1a.vqh"
  static final int TESTSIZE=40;

  static float[] test1={
    0.105939,
    0.215373,
    0.429117,
    0.587974,

    0.181173,
    0.296583,
    0.515707,
    0.715261,

    0.162327,
    0.263834,
    0.342876,
    0.406025,

    0.103571,
    0.223561,
    0.368513,
    0.540313,

    0.136672,
    0.395882,
    0.587183,
    0.652476,

    0.114338,
    0.417300,
    0.525486,
    0.698679,

    0.147492,
    0.324481,
    0.643089,
    0.757582,

    0.139556,
    0.215795,
    0.324559,
    0.399387,

    0.120236,
    0.267420,
    0.446940,
    0.608760,

    0.115587,
    0.287234,
    0.571081,
    0.708603,
  };

  static float[] test2={
    0.088654,
    0.165742,
    0.279013,
    0.395894,

    0.110812,
    0.218422,
    0.283423,
    0.371719,

    0.136985,
    0.186066,
    0.309814,
    0.381521,

    0.123925,
    0.211707,
    0.314771,
    0.433026,

    0.088619,
    0.192276,
    0.277568,
    0.343509,

    0.068400,
    0.132901,
    0.223999,
    0.302538,

    0.202159,
    0.306131,
    0.360362,
    0.416066,

    0.072591,
    0.178019,
    0.304315,
    0.376516,

    0.094336,
    0.188401,
    0.325119,
    0.390264,

    0.091636,
    0.223099,
    0.282899,
    0.375124,
  };

  static float[] test3={
    0,1,-2,3,4,-5,6,7,8,9,
    8,-2,7,-1,4,6,8,3,1,-9,
    10,11,12,13,14,15,26,17,18,19,
    30,-25,-30,-1,-5,-32,4,3,-2,0};

//  static_codebook *testlist[]={&_vq_book_lsp20_0,
//			       &_vq_book_lsp32_0,
//			       &_vq_book_res0_1a,NULL};
  static[][] float testvec={test1,test2,test3};

  static void main(String[] arg){
    Buffer write=new Buffer();
    Buffer read=new Buffer();
    int ptr=0;
    write.writeinit();
  
    System.err.println("Testing codebook abstraction...:");

    while(testlist[ptr]!=null){
      CodeBook c=new CodeBook();
      StaticCodeBook s=new StaticCodeBook();;
      float *qv=alloca(sizeof(float)*TESTSIZE);
      float *iv=alloca(sizeof(float)*TESTSIZE);
      memcpy(qv,testvec[ptr],sizeof(float)*TESTSIZE);
      memset(iv,0,sizeof(float)*TESTSIZE);

      System.err.print("\tpacking/coding "+ptr+"... ");

      // pack the codebook, write the testvector
      write.reset();
      vorbis_book_init_encode(&c,testlist[ptr]); // get it into memory
						 // we can write
      vorbis_staticbook_pack(testlist[ptr],&write);
      System.err.print("Codebook size "+write.bytes()+" bytes... ");
      for(int i=0;i<TESTSIZE;i+=c.dim){
	vorbis_book_encodev(&c,qv+i,&write);
      }
      c.clear();
    
      System.err.print("OK.\n");
      System.err.print("\tunpacking/decoding "+ptr+"... ");

      // transfer the write data to a read buffer and unpack/read
      _oggpack_readinit(&read,_oggpack_buffer(&write),_oggpack_bytes(&write));
      if(s.unpack(read)){
	System.err.print("Error unpacking codebook.\n");
	System.exit(1);
      }
      if(vorbis_book_init_decode(&c,&s)){
	System.err.print("Error initializing codebook.\n");
	System.exit(1);
      }
      for(int i=0;i<TESTSIZE;i+=c.dim){
	if(vorbis_book_decodevs(&c,iv+i,&read,1,-1)==-1){
	  System.err.print("Error reading codebook test data (EOP).\n");
	  System.exit(1);
	}
      }
      for(int i=0;i<TESTSIZE;i++){
	if(fabs(qv[i]-iv[i])>.000001){
	  System.err.print("read ("+iv[i]+") != written ("+qv[i]+") at position ("+i+")\n");
	  System.exit(1);
	}
      }

      System.err.print("OK\n");
      ptr++;
    }
    // The above is the trivial stuff; 
    // now try unquantizing a log scale codebook
  }
*/
}

class DecodeAux{
  int[] tab;
  int[] tabl;
  int tabn;

  int[] ptr0;
  int[] ptr1;
  int   aux;        // number of tree entries
}
