
#ifndef SFORMAT_H__
#define SFORMAT_H__

typedef unsigned short uint16;
typedef unsigned long uint32;

#define FORMAT_WAVE 0
#define FORMAT_AIFF 1
#define FORMAT_NEXT 2

/* the NeXTStep sound header structure; can be big or little endian  */

typedef struct _nextstep
{
    char ns_fileid[4]; 	    /* magic number '.snd' if file is big-endian */
    uint32 ns_onset; 	    /* byte offset of first sample */
    uint32 ns_length;	    /* length of sound in bytes */
    uint32 ns_format;        /* format; see below */
    uint32 ns_sr;    	    /* sample rate */
    uint32 ns_nchans;	    /* number of channels */
    char ns_info[4];   	    /* comment */
} t_nextstep;

#define NS_FORMAT_LINEAR_16	3
#define NS_FORMAT_LINEAR_24	4
#define NS_FORMAT_FLOAT         6
#define SCALE (1./(1024. * 1024. * 1024. * 2.))

/* the WAVE header.  All Wave files are little endian.  We assume
    the "fmt" chunk comes first which is usually the case but perhaps not
    always; same for AIFF and the "COMM" chunk.   */

typedef unsigned word;
typedef unsigned long dword;

typedef struct _wave
{
    char  w_fileid[4];	    	    /* chunk id 'RIFF'            */
    uint32 w_chunksize;     	    /* chunk size                 */
    char  w_waveid[4];	    	    /* wave chunk id 'WAVE'       */
    char  w_fmtid[4];	    	    /* format chunk id 'fmt '     */
    uint32 w_fmtchunksize;   	    /* format chunk size          */
    uint16  w_fmttag;	    	    /* format tag, 1 for PCM      */
    uint16  w_nchannels;    	    /* number of channels         */
    uint32 w_samplespersec;  	    /* sample rate in hz          */
    uint32 w_navgbytespersec; 	    /* average bytes per second   */
    uint16  w_nblockalign;    	    /* number of bytes per sample */
    uint16  w_nbitspersample; 	    /* number of bits in a sample */
    char  w_datachunkid[4]; 	    /* data chunk id 'data'       */
    uint32 w_datachunksize;         /* length of data chunk       */
} t_wave;


#endif

