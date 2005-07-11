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
 * Revision 1.2  2003/04/10 19:48:31  jarnbjo
 * no message
 *
 */
 
package de.jarnbjo.util.io;

import java.io.IOException;
import de.jarnbjo.util.io.BitInputStream;

/**
 *  Representation of a node in a Huffman tree, used to read
 *  Huffman compressed codewords from e.g. a Vorbis stream.
 */

final public class HuffmanNode {

   private HuffmanNode parent;
   private int depth=0;
   protected HuffmanNode o0, o1;
   protected Integer value;
   private boolean full=false;

	/**
	 *   creates a new Huffman tree root node
	 */ 

   public HuffmanNode() {
      this(null);
   }

   protected HuffmanNode(HuffmanNode parent) {
      this.parent=parent;
      if(parent!=null) {
         depth=parent.getDepth()+1;
      }
   }

   protected HuffmanNode(HuffmanNode parent, int value) {
      this(parent);
      this.value=new Integer(value);
      full=true;
   }

   protected int read(BitInputStream bis) throws IOException {
      HuffmanNode iter=this;
      while(iter.value==null) {
         iter=bis.getBit()?iter.o1:iter.o0;
      }
      return iter.value.intValue();
   }

   protected HuffmanNode get0() {
      return o0==null?set0(new HuffmanNode(this)):o0;
   }

   protected HuffmanNode get1() {
      return o1==null?set1(new HuffmanNode(this)):o1;
   }

   protected Integer getValue() {
      return value;
   }

   private HuffmanNode getParent() {
      return parent;
   }

   protected int getDepth() {
      return depth;
   }

   private boolean isFull() {
      return full?true:(full=o0!=null&&o0.isFull()&&o1!=null&&o1.isFull());
   }

   private HuffmanNode set0(HuffmanNode value) {
      return o0=value;
   }

   private HuffmanNode set1(HuffmanNode value) {
      return o1=value;
   }

   private void setValue(Integer value) {
      full=true;
      this.value=value;
   }

	/**
	 *  creates a new tree node at the first free location at the given
	 *  depth, and assigns the value to it
	 *
	 *  @param depth the tree depth of the new node (codeword length in bits)
	 *  @param value the node's new value
    */
    
   public boolean setNewValue(int depth, int value) {
      if(isFull()) {
         return false;
      }
      if(depth==1) {
         if(o0==null) {
            set0(new HuffmanNode(this, value));
            return true;
         }
         else if(o1==null) {
            set1(new HuffmanNode(this, value));
            return true;
         }
         else {
            return false;
         }
      }
      else {
         return get0().setNewValue(depth-1, value)?
            true:
            get1().setNewValue(depth-1, value);
      }
   }
}
