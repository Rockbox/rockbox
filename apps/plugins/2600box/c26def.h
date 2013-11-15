#ifndef C26DEF_H
#define C26DEF_H

/* 
   Common 2600 (.c26) format v1.0 specification
   -------------------------------------
   Alex Hornby, ahornby@zetnet.co.uk


   Introduction
   ============

   Common 2600 file format definitions. For discussion and suggestions for 
   improvement, e-mail ahornby@zetnet.co.uk. I would like to see a fully
   comprehensive 2600 file format develop so please copy this structure
   and use it in your emulators.

   The format has been developed due to the multitude of different banking
   schemes for 2600 cartridges, along with the need to select an appropriate
   control device for each game. Using the .c26 format you will be able to
   load games without giving loads of command line switches. 

   Philosophy
   ==========
   To avoid the format splitting into several competing ones, please do
   not alter the format without discussing it first. I'm not trying to be
   bossy, just to keep the common format truly common. 

   Tags
   ====
   The format is tagged so as to be extensible and allow both forward and
   backward compatibility. It also means that information that is not 
   needed or known does not have to be stored. e.g. If the cartridge image
   is not a saved game then I do not need the game state tags.

   The format is a system of tags each being a tag type and the length of
   data in that section. If a tag is not recognised then it should
   be ignored. Each tag is a zero terminated string followed by a 32bit 
   signed integer describing the length. If the tag is small the the length
   integer can constitute the data item.

   Case is NOT important in tag names 

   Cross Platform Notes
   ====================
   Note that integers are stored in the Intel/DEC Alpha style. 
   All strings are zero terminated .
*/

/* 
   Defined TAGS
   ============
   
   + Audit tags: All files should include these tags at the start of the file. 
      
   _2600_VERSION: Gives file format version as an integer. Currently 1
   WRITER: Name of program that wrote file.
   
   + Cartridge Information tags: useful for collectors.
   
   CARTNAME: Name of cartridge.
   CARTMAN:  Manufacturer of cartridge.
   CARTAUTHOR: Name of programmer/programming team who wrote cartridge.
   CARTSERIAL: Serial number of the cartridge.
   
   + Cartridge operation tags: necessary for the running of the game.   
   
   TVTYPE:     integer, 0=NTSC 1=PAL.
   CONTROLLERS: Left controller BYTE then  Right controller BYTE.
   BANKING:    Bank switching scheme.
   DATA:       Cartridge ROM data.
   
   + Game state tags: used for saved games.
   
   CPUREGS:  CPU registers.
   GAMEREGS: TIA and PIA registers.
   PIARAM:   The 128 bytes of RAM from the PIA.

   CARTRAM:  Cartridge RAM, if supported by the BANKING type
   
   */
enum TAGTYPE { _2600_VERSION=-1, WRITER=1, 
               CARTNAME=2, CARTMAN=3, CARTAUTHOR=4, CARTSERIAL=5,
               TVTYPE=-2, CONTROLLERS=-3, BANKING=-4, DATA=6,
               CPUREGS=7, GAMEREGS=8, PIARAM=9, CARTRAM=10 };

char *tag_desc[]={ "_2600_VERSION", "WRITER", 
		   "CARTNAME", "CARTMAN", "CARTAUTHOR", "CARTSERIAL",
		   "TVTYPE", "CONTROLLERS", "BANKING", "DATA",
		   "CPUREGS", "GAMEREGS", "PIARAM", "CARTRAM"};


/* Tag structure */

struct c26_tag {
    int type;
    int len;
};


#endif
