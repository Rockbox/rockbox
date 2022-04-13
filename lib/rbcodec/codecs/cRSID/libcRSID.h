// cRSID lightweight RealSID (integer-only) library-header (with API-calls) by Hermit (Mihaly Horvath)

#ifndef LIBCRSID_HEADER
#define LIBCRSID_HEADER //used  to prevent double inclusion of this header-file


enum cRSID_Specifications { CRSID_SIDCOUNT_MAX=3, CRSID_CIACOUNT=2 };
enum cRSID_StatusCodes    { CRSID_STATUS_OK=0, CRSID_ERROR_INIT=-1, CRSID_ERROR_LOAD=-2 };


typedef struct cRSID_SIDheader   cRSID_SIDheader;
typedef struct cRSID_C64instance cRSID_C64instance;


extern cRSID_C64instance cRSID_C64; //the only global object (for faster & simpler access than with struct-pointers, in some places)


// Main API functions (mainly in libcRSID.c)
cRSID_C64instance* cRSID_init           (unsigned short samplerate, unsigned short buflen); //init emulation objects and sound
#ifdef CRSID_PLATFORM_PC
char               cRSID_playSIDfile    (cRSID_C64instance* C64, char* filename, char subtune); //simple single-call SID playback
cRSID_SIDheader*   cRSID_loadSIDtune    (cRSID_C64instance* C64, char* filename); //load and process SID-filedata to C64 memory
void               cRSID_playSIDtune    (void); //start/continue playback (enable playing audio-buffer)
void               cRSID_pauseSIDtune   (void); //pause playback (enable playing audio-buffer)
void               cRSID_close          (void); //close sound etc.
#endif
cRSID_SIDheader*   cRSID_processSIDfile (cRSID_C64instance* C64, unsigned char* filedata, int filesize); //in host/file.c, copy SID-data to C64 memory
void               cRSID_initSIDtune    (cRSID_C64instance* C64, cRSID_SIDheader* SIDheader, char subtune); //init tune/subtune
signed short       cRSID_generateSample (cRSID_C64instance* C64); //in host/audio.c, calculate a single sample


struct cRSID_SIDheader {              //Offset:   default/info:
 unsigned char MagicString[4];            //$00 - "PSID" or "RSID" (RSID must provide Reset-circumstances & CIA/VIC-interrupts)
 unsigned char VersionH00;                //$04
 unsigned char Version;                   //$05 - 1 for PSID v1, 2..4 for PSID v2..4 or RSID v2..4 (3/4 has 2SID/3SID support)
 unsigned char HeaderSizeH00;             //$06
 unsigned char HeaderSize;                //$07 - $76 for v1, $7C for v2..4
 unsigned char LoadAddressH,LoadAddressL; //$08 - if 0 it's a PRG and its loadaddress is used (RSID: 0, PRG-loadaddress>=$07E8)
 unsigned char InitAddressH,InitAddressL; //$0A - if 0 it's taken from load-address (but should be set) (RSID: don't point to ROM, 0 if BASICflag set)
 unsigned char PlayAddressH,PlayAddressL; //$0C - if 0 play-routine-call is set by the initializer (always true for RSID)
 unsigned char SubtuneAmountH00;          //$0E
 unsigned char SubtuneAmount;             //$0F - 1..256
 unsigned char DefaultSubtuneH00;         //$10
 unsigned char DefaultSubtune;            //$11 - 1..256 (optional, defaults to 1)
 unsigned char SubtuneTimeSources[4];     //$12 - 0:Vsync / 1:CIA1 (for PSID) (LSB is subtune1, MSB above 32) , always 0 for RSID
 char          Title[32];                 //$16 - strings are using 1252 codepage
 char          Author[32];                //$36
 char          ReleaseInfo[32];           //$56
 //SID v2 additions:                              (if SID2/SID3 model is set to unknown, they're set to the same model as SID1)
 unsigned char ModelFormatStandardH;      //$76 - bit9&8/7&6/5&4: SID3/2/1 model (00:?,01:6581,10:8580,11:both), bit3&2:VideoStandard..
 unsigned char ModelFormatStandard;       //$77 ..(01:PAL,10:NTSC,11:both), bit1:(0:C64,1:PlaySIDsamples/RSID_BASICflag), bit0:(0:builtin-player,1:MUS)
 unsigned char RelocStartPage;            //$78 - v2NG specific, if 0 the SID doesn't write outside its data-range, if $FF there's no place for driver
 unsigned char RelocFreePages;            //$79 - size of area from RelocStartPage for driver-relocation (RSID: must not contain ROM or 0..$3FF)
 unsigned char SID2baseAddress;           //$7A - (SID2BASE-$d000)/16 //SIDv3-relevant, only $42..$FE values are valid ($d420..$DFE0), else no SID2
 unsigned char SID3baseAddress;           //$7B - (SID3BASE-$d000)/16 //SIDv4-relevant, only $42..$FE values are valid ($d420..$DFE0), else no SID3
}; //music-program follows right after the header


#endif //LIBCRSID_HEADER
