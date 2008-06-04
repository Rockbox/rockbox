/*
  Copyright (c) 2005, The Musepack Development Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

  * Neither the name of the The Musepack Development Team nor the
  names of its contributors may be used to endorse or promote
  products derived from this software without specific prior
  written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/// \file mpc_decoder.c
/// Core decoding routines and logic.

#include "musepack.h"
#include "internal.h"
#include "requant.h"
#include "huffman.h"

//SV7 tables
extern const HuffmanTyp*   mpc_table_HuffQ [2] [8];
extern const HuffmanTyp    mpc_table_HuffHdr  [10];
extern const HuffmanTyp    mpc_table_HuffSCFI [ 4];
extern const HuffmanTyp    mpc_table_HuffDSCF [16];


#ifdef MPC_SUPPORT_SV456
//SV4/5/6 tables
extern const HuffmanTyp*   mpc_table_SampleHuff [18];
extern const HuffmanTyp    mpc_table_SCFI_Bundle   [ 8];
extern const HuffmanTyp    mpc_table_DSCF_Entropie [13];
extern const HuffmanTyp    mpc_table_Region_A [16];
extern const HuffmanTyp    mpc_table_Region_B [ 8];
extern const HuffmanTyp    mpc_table_Region_C [ 4];

#endif

#ifndef MPC_LITTLE_ENDIAN
#define SWAP(X) mpc_swap32(X)
#else
#define SWAP(X) X
#endif

#ifdef SCF_HACK
#define SCF_DIFF(SCF, D) (SCF == -128 ? -128 : SCF + D)
#else 
#define SCF_DIFF(SCF, D) SCF + D
#endif

#define LOOKUP(x, e, q)   mpc_decoder_make_huffman_lookup ( (q), sizeof(q), (x), (e) )
#define Decode_DSCF()   HUFFMAN_DECODE_FASTEST ( d, mpc_table_HuffDSCF, LUTDSCF, 6 )
#define HUFFMAN_DECODE_FASTEST(d,a,b,c)  mpc_decoder_huffman_decode_fastest ( (d), (a), (b), 32-(c) )
#define HUFFMAN_DECODE_FASTERER(d,a,b,c)  mpc_decoder_huffman_decode_fasterer ( (d), (a), (b), 32-(c) )

mpc_uint8_t     LUT1_0  [1<< 6] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT1_1  [1<< 9] IBSS_ATTR_MPC_LARGE_IRAM; //  576 Bytes
mpc_uint8_t     LUT2_0  [1<< 7] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT2_1  [1<<10] IBSS_ATTR_MPC_LARGE_IRAM; // 1152 Bytes
mpc_uint8_t     LUT3_0  [1<< 4] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT3_1  [1<< 5] IBSS_ATTR_MPC_LARGE_IRAM; //   48 Bytes
mpc_uint8_t     LUT4_0  [1<< 4] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT4_1  [1<< 5] IBSS_ATTR_MPC_LARGE_IRAM; //   48 Bytes
mpc_uint8_t     LUT5_0  [1<< 6] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT5_1  [1<< 8] IBSS_ATTR_MPC_LARGE_IRAM; //  320 Bytes
mpc_uint8_t     LUT6_0  [1<< 7] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT6_1  [1<< 7] IBSS_ATTR_MPC_LARGE_IRAM; //  256 Bytes
mpc_uint8_t     LUT7_0  [1<< 8] IBSS_ATTR_MPC_LARGE_IRAM;
mpc_uint8_t     LUT7_1  [1<< 8] IBSS_ATTR_MPC_LARGE_IRAM; //  512 Bytes
mpc_uint8_t     LUTDSCF [1<< 6] IBSS_ATTR_MPC_LARGE_IRAM; //   64 Bytes = 2976 Bytes

//------------------------------------------------------------------------------
// types
//------------------------------------------------------------------------------
enum
    {
        SEEK_PRE_DECODE = 33,               // number of frames to be pre-decoded
        MEMSIZE = MPC_DECODER_MEMSIZE,      // overall buffer size
        MEMSIZE2 = (MEMSIZE/2),             // size of one buffer
        MEMMASK = (MEMSIZE-1)
    };

//------------------------------------------------------------------------------
// forward declarations
//------------------------------------------------------------------------------
void mpc_decoder_read_bitstream_sv6(mpc_decoder *d);
void mpc_decoder_read_bitstream_sv7(mpc_decoder *d, mpc_bool_t fastSeeking);
void mpc_decoder_update_buffer(mpc_decoder *d);
mpc_bool_t mpc_decoder_seek_sample(mpc_decoder *d, mpc_int64_t destsample);
void mpc_decoder_requantisierung(mpc_decoder *d, const mpc_int32_t Last_Band);
void mpc_decoder_seek_to(mpc_decoder *d, mpc_uint32_t bitPos);
void mpc_decoder_seek_forward(mpc_decoder *d, mpc_uint32_t bits);
mpc_uint32_t mpc_decoder_jump_frame(mpc_decoder *d);
void mpc_decoder_fill_buffer(mpc_decoder *d);
void mpc_decoder_reset_state(mpc_decoder *d);
static mpc_uint32_t get_initial_fpos(mpc_decoder *d, mpc_uint32_t StreamVersion);
static inline mpc_int32_t mpc_decoder_huffman_decode_fastest(mpc_decoder *d, const HuffmanTyp* Table, const mpc_uint8_t* tab, mpc_uint16_t unused_bits);
static void mpc_move_next(mpc_decoder *d);

mpc_uint32_t  Seekbuffer[MPC_SEEK_BUFFER_SIZE];
mpc_uint32_t  Speicher[MPC_DECODER_MEMSIZE];
MPC_SAMPLE_FORMAT Y_L[36][32] IBSS_ATTR_MPC_LARGE_IRAM;
MPC_SAMPLE_FORMAT Y_R[36][32] IBSS_ATTR_MPC_LARGE_IRAM;

//------------------------------------------------------------------------------
// utility functions
//------------------------------------------------------------------------------
static mpc_int32_t f_read(mpc_decoder *d, void *ptr, size_t size) 
{ 
    return d->r->read(d->r->data, ptr, size); 
};

static mpc_bool_t f_seek(mpc_decoder *d, mpc_int32_t offset) 
{ 
    return d->r->seek(d->r->data, offset); 
};

static mpc_int32_t f_read_dword(mpc_decoder *d, mpc_uint32_t * ptr, mpc_uint32_t count) 
{
    count = f_read(d, ptr, count << 2) >> 2;
    return count;
}

//------------------------------------------------------------------------------
// huffman & bitstream functions
//------------------------------------------------------------------------------
static const mpc_uint32_t mask [33] ICONST_ATTR = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
    0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
    0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
    0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
    0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
    0xFFFFFFFF
};

/* F U N C T I O N S */

// resets bitstream decoding
static void
mpc_decoder_reset_bitstream_decode(mpc_decoder *d) 
{
    d->dword = 0;
    d->next = 0;
    d->pos = 0;
    d->Zaehler = 0;
    d->WordsRead = 0;
}

// reports the number of read bits
static mpc_uint32_t
mpc_decoder_bits_read(mpc_decoder *d) 
{
    return 32 * d->WordsRead + d->pos;
}

static void mpc_move_next(mpc_decoder *d) {
    d->Zaehler = (d->Zaehler + 1) & MEMMASK;
    d->dword = d->next;
    d->next = SWAP(d->Speicher[(d->Zaehler + 1) & MEMMASK]);
    d->pos -= 32;
    ++(d->WordsRead);
}

// read desired number of bits out of the bitstream
static inline mpc_uint32_t
mpc_decoder_bitstream_read(mpc_decoder *d, const mpc_uint32_t bits) 
{
    mpc_uint32_t out = d->dword;

    d->pos += bits;

    if (d->pos < 32) {
        out >>= (32 - d->pos);
    }
    else {
        mpc_move_next(d);
        if (d->pos) {
            out <<= d->pos;
            out |= d->dword >> (32 - d->pos);
        }
    }

    return out & mask[bits];
}

static void 
mpc_decoder_make_huffman_lookup(
    mpc_uint8_t* lookup, size_t length, const HuffmanTyp* Table, size_t elements )
{
    size_t    i;
    size_t    idx  = elements;
    mpc_uint32_t  dval = (mpc_uint32_t)0x80000000L / length * 2;
    mpc_uint32_t  val  = dval - 1;

    for ( i = 0; i < length; i++, val += dval ) {
        while ( idx > 0  &&  val >= Table[idx-1].Code )
            idx--;
        *lookup++ = (mpc_uint8_t)idx;
    }

    return;
}

#ifdef MPC_SUPPORT_SV456
// decode SCFI-bundle (sv4,5,6)
static void
mpc_decoder_scfi_bundle_read(
    mpc_decoder *d,
    const HuffmanTyp* Table, mpc_int8_t* SCFI, mpc_bool_t* DSCF) 
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;

    if (d->pos > 26) {
        code |= d->next >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        mpc_move_next(d);
    }

    *SCFI = Table->Value >> 1;
    *DSCF = Table->Value &  1;
}

// basic huffman decoding routine
// works with maximum lengths up to 14
static mpc_int32_t
mpc_decoder_huffman_decode(mpc_decoder *d, const HuffmanTyp *Table) 
{
    // load preview and decode
    mpc_uint32_t code = d->dword << d->pos;
    
    if (d->pos > 18) {
        code |= d->next >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        mpc_move_next(d);
    }

    return Table->Value;
}
#endif

// faster huffman through previewing less bits
// works with maximum lengths up to 10
static mpc_int32_t
mpc_decoder_huffman_decode_fast(mpc_decoder *d, const HuffmanTyp* Table)
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;
    
    if (d->pos > 22) {
        code |= d->next >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        mpc_move_next(d);
    }

    return Table->Value;
}

// even faster huffman through previewing even less bits
// works with maximum lengths up to 5
static mpc_int32_t
mpc_decoder_huffman_decode_faster(mpc_decoder *d, const HuffmanTyp* Table)
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;
    
    if (d->pos > 27) {
        code |= d->next >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        mpc_move_next(d);
    }

    return Table->Value;
}

/* partial lookup table decode */
static mpc_int32_t
mpc_decoder_huffman_decode_fasterer(mpc_decoder *d, const HuffmanTyp* Table, const mpc_uint8_t* tab, mpc_uint16_t unused_bits)
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;
    
    if (d->pos > 18) { // preview 14 bits
        code |= d->next >> (32 - d->pos);
    }

    Table += tab [(size_t)(code >> unused_bits) ];

    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        mpc_move_next(d);
    }

    return Table->Value;
}

/* full decode using lookup table */
static inline mpc_int32_t
mpc_decoder_huffman_decode_fastest(mpc_decoder *d, const HuffmanTyp* Table, const mpc_uint8_t* tab, mpc_uint16_t unused_bits)
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;

    if (d->pos > unused_bits) {
        code |= d->next >> (32 - d->pos);
    }

    Table+=tab [(size_t)(code >> unused_bits) ];

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        mpc_move_next(d);
    }

    return Table->Value;
}

static void
mpc_decoder_reset_v(mpc_decoder *d) 
{
    memset(d->V_L, 0, sizeof d->V_L);
    memset(d->V_R, 0, sizeof d->V_R);
}

static void
mpc_decoder_reset_synthesis(mpc_decoder *d) 
{
    mpc_decoder_reset_v(d);
}

static void
mpc_decoder_reset_y(mpc_decoder *d) 
{
    memset(d->Y_L, 0, sizeof Y_L);
    memset(d->Y_R, 0, sizeof Y_R);
}

static void
mpc_decoder_reset_globals(mpc_decoder *d) 
{
    mpc_decoder_reset_bitstream_decode(d);

    d->DecodedFrames    = 0;
    d->MaxDecodedFrames = 0;
    d->StreamVersion    = 0;
    d->MS_used          = 0;

    memset(d->Y_L          , 0, sizeof Y_L           );
    memset(d->Y_R          , 0, sizeof Y_R           );
    memset(d->SCF_Index_L  , 0, sizeof d->SCF_Index_L);
    memset(d->SCF_Index_R  , 0, sizeof d->SCF_Index_R);
    memset(d->Res_L        , 0, sizeof d->Res_L      );
    memset(d->Res_R        , 0, sizeof d->Res_R      );
    memset(d->SCFI_L       , 0, sizeof d->SCFI_L     );
    memset(d->SCFI_R       , 0, sizeof d->SCFI_R     );
#ifdef MPC_SUPPORT_SV456
    memset(d->DSCF_Flag_L  , 0, sizeof d->DSCF_Flag_L);
    memset(d->DSCF_Flag_R  , 0, sizeof d->DSCF_Flag_R);
#endif
    memset(d->Q            , 0, sizeof d->Q          );
    memset(d->MS_Flag      , 0, sizeof d->MS_Flag    );
}

mpc_uint32_t
mpc_decoder_decode_frame(mpc_decoder *d, mpc_uint32_t *in_buffer,
                         mpc_uint32_t in_len, MPC_SAMPLE_FORMAT *out_buffer)
{
  mpc_decoder_reset_bitstream_decode(d);
  if (in_len > sizeof(Speicher)) in_len = sizeof(Speicher);
  memcpy(d->Speicher, in_buffer, in_len);
  d->dword = SWAP(d->Speicher[0]);
  d->next = SWAP(d->Speicher[1]);
  switch (d->StreamVersion) {
#ifdef MPC_SUPPORT_SV456
    case 0x04:
    case 0x05:
    case 0x06:
        mpc_decoder_read_bitstream_sv6(d);
        break;
#endif
    case 0x07:
    case 0x17:
        mpc_decoder_read_bitstream_sv7(d, FALSE);
        break;
    default:
        return (mpc_uint32_t)(-1);
  }
  mpc_decoder_requantisierung(d, d->Max_Band);
  mpc_decoder_synthese_filter_float(d, out_buffer);
  return mpc_decoder_bits_read(d);
}

static mpc_uint32_t
mpc_decoder_decode_internal(mpc_decoder *d, MPC_SAMPLE_FORMAT *buffer) 
{
    mpc_uint32_t output_frame_length = MPC_FRAME_LENGTH;

    mpc_uint32_t  FrameBitCnt = 0;

    // output the last part of the last frame here, if needed
    if (d->last_block_samples > 0) {
        output_frame_length = d->last_block_samples;
        d->last_block_samples = 0; // it's going to be handled now, so reset it 
        if (!d->TrueGaplessPresent) {
            mpc_decoder_reset_y(d);
        } else {
            mpc_decoder_bitstream_read(d, 20);
            mpc_decoder_read_bitstream_sv7(d, FALSE);
            mpc_decoder_requantisierung(d, d->Max_Band);
        }
        mpc_decoder_synthese_filter_float(d, buffer);
        return output_frame_length;
    }
    
    if (d->DecodedFrames >= d->OverallFrames) {
        return (mpc_uint32_t)(-1);                           // end of file -> abort decoding
    }

    if (d->DecodedFrames == 0)
    {
        d->SeekTable[0] = mpc_decoder_bits_read(d);
        d->SeekTableCounter = 0;
    }

    // read jump-info for validity check of frame
    d->FwdJumpInfo  = mpc_decoder_bitstream_read(d, 20);

    d->ActDecodePos = (d->Zaehler << 5) + d->pos;

    // decode data and check for validity of frame
    FrameBitCnt = mpc_decoder_bits_read(d);
    switch (d->StreamVersion) {
#ifdef MPC_SUPPORT_SV456
    case 0x04:
    case 0x05:
    case 0x06:
        mpc_decoder_read_bitstream_sv6(d);
        break;
#endif
    case 0x07:
    case 0x17:
        mpc_decoder_read_bitstream_sv7(d, FALSE);
        break;
    default:
        return (mpc_uint32_t)(-1);
    }
    d->FrameWasValid = mpc_decoder_bits_read(d) - FrameBitCnt == d->FwdJumpInfo;

    d->DecodedFrames++;

    /* update seek table */
    d->SeekTableCounter += d->FwdJumpInfo + 20;
    if (0 == ((d->DecodedFrames) & (d->SeekTable_Mask))) 
    {
        d->SeekTable[d->DecodedFrames>>d->SeekTable_Step] = d->SeekTableCounter;
        d->MaxDecodedFrames = d->DecodedFrames;
        d->SeekTableCounter = 0;
    }

    // synthesize signal
    mpc_decoder_requantisierung(d, d->Max_Band);

    mpc_decoder_synthese_filter_float(d, buffer);

    // cut off first MPC_DECODER_SYNTH_DELAY zero-samples
    if (d->DecodedFrames == d->OverallFrames  && d->StreamVersion >= 6) {        
        // reconstruct exact filelength
        mpc_int32_t  mod_block   = mpc_decoder_bitstream_read(d,  11);
        mpc_int32_t  FilterDecay;

        if (mod_block == 0) {
            // Encoder bugfix
            mod_block = 1152;                    
        }
        FilterDecay = (mod_block + MPC_DECODER_SYNTH_DELAY) % MPC_FRAME_LENGTH;

        // additional FilterDecay samples are needed for decay of synthesis filter
        if (MPC_DECODER_SYNTH_DELAY + mod_block >= MPC_FRAME_LENGTH) {
            // this variable will be checked for at the top of the function
            d->last_block_samples = FilterDecay;
        }
        else { // there are only FilterDecay samples needed for this frame
            output_frame_length = FilterDecay;
        }
    }

    if (d->samples_to_skip) {
        if (output_frame_length < d->samples_to_skip) {
            d->samples_to_skip -= output_frame_length;
            output_frame_length = 0;
        }
        else {
            output_frame_length -= d->samples_to_skip;
            memmove(
                buffer, 
                buffer + d->samples_to_skip, 
                output_frame_length * sizeof (MPC_SAMPLE_FORMAT));
            memmove(
                buffer + MPC_FRAME_LENGTH, 
                buffer + MPC_FRAME_LENGTH + d->samples_to_skip, 
                output_frame_length * sizeof (MPC_SAMPLE_FORMAT));
            d->samples_to_skip = 0;
        }
    }

    return output_frame_length;
}

mpc_uint32_t mpc_decoder_decode(
    mpc_decoder *d,
    MPC_SAMPLE_FORMAT *buffer, 
    mpc_uint32_t *vbr_update_acc, 
    mpc_uint32_t *vbr_update_bits)
{
    for(;;)
    {
        mpc_uint32_t RING = d->Zaehler;
        mpc_int32_t vbr_ring = (RING << 5) + d->pos;

        mpc_uint32_t valid_samples = mpc_decoder_decode_internal(d, buffer);

        if (valid_samples == (mpc_uint32_t)(-1) ) {
            return 0;
        }

        /**************** ERROR CONCEALMENT *****************/
        if (d->FrameWasValid == 0 ) {
            // error occurred in bitstream
            return (mpc_uint32_t)(-1);
        } 
        else {
            if (vbr_update_acc && vbr_update_bits) {
                (*vbr_update_acc) ++;
                vbr_ring = (d->Zaehler << 5) + d->pos - vbr_ring;
                if (vbr_ring < 0) {
                    vbr_ring += 524288;
                }
                (*vbr_update_bits) += vbr_ring;
            }

        }
        mpc_decoder_update_buffer(d);

        if (valid_samples > 0) {
            return valid_samples;
        }
    }
}

void
mpc_decoder_requantisierung(mpc_decoder *d, const mpc_int32_t Last_Band) 
{
    mpc_int32_t     Band;
    mpc_int32_t     n;
    MPC_SAMPLE_FORMAT facL;
    MPC_SAMPLE_FORMAT facR;
    MPC_SAMPLE_FORMAT templ;
    MPC_SAMPLE_FORMAT tempr;
    MPC_SAMPLE_FORMAT* YL;
    MPC_SAMPLE_FORMAT* YR;
    mpc_int16_t*    L;
    mpc_int16_t*    R;

#ifdef MPC_FIXED_POINT
#if MPC_FIXED_POINT_FRACTPART == 14
#define MPC_MULTIPLY_SCF(CcVal, SCF_idx) \
    MPC_MULTIPLY_EX(CcVal, d->SCF[SCF_idx], d->SCF_shift[SCF_idx])
#else

#error FIXME, Cc table is in 18.14 format

#endif
#else
#define MPC_MULTIPLY_SCF(CcVal, SCF_idx) \
    MPC_MULTIPLY(CcVal, d->SCF[SCF_idx])
#endif
    // requantization and scaling of subband-samples
    for ( Band = 0; Band <= Last_Band; Band++ ) {   // setting pointers
        YL = d->Y_L[0] + Band;
        YR = d->Y_R[0] + Band;
        L  = d->Q[Band].L;
        R  = d->Q[Band].R;
        /************************** MS-coded **************************/
        if ( d->MS_Flag [Band] ) {
            if ( d->Res_L [Band] ) {
                if ( d->Res_R [Band] ) {    // M!=0, S!=0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = MPC_MULTIPLY_FLOAT_INT(facL,*L++))+(tempr = MPC_MULTIPLY_FLOAT_INT(facR,*R++));
                        *YR   = templ - tempr;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = MPC_MULTIPLY_FLOAT_INT(facL,*L++))+(tempr = MPC_MULTIPLY_FLOAT_INT(facR,*R++));
                        *YR   = templ - tempr;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = MPC_MULTIPLY_FLOAT_INT(facL,*L++))+(tempr = MPC_MULTIPLY_FLOAT_INT(facR,*R++));
                        *YR   = templ - tempr;
                    }
                } else {    // M!=0, S==0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                    }
                }
            } else {
                if (d->Res_R[Band])    // M==0, S!=0
                {
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = MPC_MULTIPLY_FLOAT_INT(facR,*(R++)));
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = MPC_MULTIPLY_FLOAT_INT(facR,*(R++)));
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = MPC_MULTIPLY_FLOAT_INT(facR,*(R++)));
                    }
                } else {    // M==0, S==0
                    for ( n = 0; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = 0;
                    }
                }
            }
        }
        /************************** LR-coded **************************/
        else {
            if ( d->Res_L [Band] ) {
                if ( d->Res_R [Band] ) {    // L!=0, R!=0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for (n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for (; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for (; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                } else {     // L!=0, R==0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = 0;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = 0;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = 0;
                    }
                }
            }
            else {
                if ( d->Res_R [Band] ) {    // L==0, R!=0
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = 0;
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = 0;
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = 0;
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                } else {    // L==0, R==0
                    for ( n = 0; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = 0;
                    }
                }
            }
        }
    }
}

#ifdef MPC_SUPPORT_SV456
static const unsigned char Q_res[32][16] ICONST_ATTR = {
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,17},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,6,17,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,17,0,0,0,0,0,0,0,0,0,0,0,0},
};

/****************************************** SV 6 ******************************************/
void
mpc_decoder_read_bitstream_sv6(mpc_decoder *d) 
{
    mpc_int32_t n,k;
    mpc_int32_t Max_used_Band=0;
    const HuffmanTyp *Table;
    const HuffmanTyp *x1;
    const HuffmanTyp *x2;
    mpc_int8_t *L;
    mpc_int8_t *R;
    mpc_int16_t *QL;
    mpc_int16_t *QR;
    mpc_int8_t *ResL = d->Res_L;
    mpc_int8_t *ResR = d->Res_R;

    /************************ HEADER **************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    for (n=0; n <= d->Max_Band; ++n, ++ResL, ++ResR)
    {
        if      (n<11)           Table = mpc_table_Region_A;
        else if (n>=11 && n<=22) Table = mpc_table_Region_B;
        else /*if (n>=23)*/      Table = mpc_table_Region_C;

        *ResL = Q_res[n][mpc_decoder_huffman_decode(d, Table)];
        if (d->MS_used) {
            d->MS_Flag[n] = mpc_decoder_bitstream_read(d,  1);
        }
        *ResR = Q_res[n][mpc_decoder_huffman_decode(d, Table)];

        // only perform the following procedure up to the maximum non-zero subband
        if (*ResL || *ResR) Max_used_Band = n;
    }

    /************************* SCFI-Bundle *****************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR) {
        if (*ResL) mpc_decoder_scfi_bundle_read(d, mpc_table_SCFI_Bundle, &(d->SCFI_L[n]), &(d->DSCF_Flag_L[n]));
        if (*ResR) mpc_decoder_scfi_bundle_read(d, mpc_table_SCFI_Bundle, &(d->SCFI_R[n]), &(d->DSCF_Flag_R[n]));
    }

    /***************************** SCFI ********************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    L    = d->SCF_Index_L[0];
    R    = d->SCF_Index_R[0];
    for (n=0; n <= Max_used_Band; ++n, ++ResL, ++ResR, L+=3, R+=3)
    {
        if (*ResL)
        {
            /*********** DSCF ************/
            if (d->DSCF_Flag_L[n]==1)
            {
                switch (d->SCFI_L[n])
                {
                case 3:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 1:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    L[1] = L[0] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    L[2] = L[1];
                    break;
                case 2:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    L[1] = L[0];
                    L[2] = L[1] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    break;
                case 0:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    L[1] = L[0] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    L[2] = L[1] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    break;
                default:
                    return;
                    break;
                }
            }
            /************ SCF ************/
            else
            {
                switch (d->SCFI_L[n])
                {
                case 3:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 1:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = mpc_decoder_bitstream_read(d,  6);
                    L[2] = L[1];
                    break;
                case 2:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = L[0];
                    L[2] = mpc_decoder_bitstream_read(d,  6);
                    break;
                case 0:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = mpc_decoder_bitstream_read(d,  6);
                    L[2] = mpc_decoder_bitstream_read(d,  6);
                    break;
                default:
                    return;
                    break;
                }
            }
        }
        if (*ResR)
        {
            /*********** DSCF ************/
            if (d->DSCF_Flag_R[n]==1)
            {
                switch (d->SCFI_R[n])
                {
                case 3:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 1:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    R[1] = R[0] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    R[2] = R[1];
                    break;
                case 2:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    R[1] = R[0];
                    R[2] = R[1] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    break;
                case 0:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    R[1] = R[0] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    R[2] = R[1] + mpc_decoder_huffman_decode_fast(d,  mpc_table_DSCF_Entropie);
                    break;
                default:
                    return;
                    break;
                }
            }
            /************ SCF ************/
            else
            {
                switch (d->SCFI_R[n])
                {
                case 3:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 1:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = mpc_decoder_bitstream_read(d, 6);
                    R[2] = R[1];
                    break;
                case 2:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = R[0];
                    R[2] = mpc_decoder_bitstream_read(d, 6);
                    break;
                case 0:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = mpc_decoder_bitstream_read(d, 6);
                    R[2] = mpc_decoder_bitstream_read(d, 6);
                    break;
                default:
                    return;
                    break;
                }
            }
        }
    }

    /**************************** Samples ****************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    for (n=0; n <= Max_used_Band; ++n, ++ResL, ++ResR)
    {
        // setting pointers
        x1 = mpc_table_SampleHuff[*ResL];
        x2 = mpc_table_SampleHuff[*ResR];
        QL = d->Q[n].L;
        QR = d->Q[n].R;

        if (x1!=NULL || x2!=NULL)
            for (k=0; k<36; ++k)
            {
                if (x1 != NULL) *QL++ = mpc_decoder_huffman_decode_fast(d,  x1);
                if (x2 != NULL) *QR++ = mpc_decoder_huffman_decode_fast(d,  x2);
            }

        if (*ResL>7 || *ResR>7)
            for (k=0; k<36; ++k)
            {
                if (*ResL>7) *QL++ = (mpc_int16_t)mpc_decoder_bitstream_read(d,  Res_bit[*ResL]) - Dc[*ResL];
                if (*ResR>7) *QR++ = (mpc_int16_t)mpc_decoder_bitstream_read(d,  Res_bit[*ResR]) - Dc[*ResR];
            }
    }
}
#endif //MPC_SUPPORT_SV456
/****************************************** SV 7 ******************************************/
void
mpc_decoder_read_bitstream_sv7(mpc_decoder *d, mpc_bool_t fastSeeking) 
{
    mpc_int32_t n,k;
    mpc_int32_t Max_used_Band=0;
    const HuffmanTyp *Table;
    mpc_int32_t idx;
    mpc_int8_t *L   ,*R;
    mpc_int16_t *LQ   ,*RQ;
    mpc_int8_t *ResL,*ResR;
    mpc_uint32_t tmp;
    mpc_uint8_t *LUT;
    mpc_uint8_t max_length;

    /***************************** Header *****************************/
    ResL  = d->Res_L;
    ResR  = d->Res_R;

    // first subband
    *ResL = mpc_decoder_bitstream_read(d, 4);
    *ResR = mpc_decoder_bitstream_read(d, 4);
    if (d->MS_used && !(*ResL==0 && *ResR==0)) {
        d->MS_Flag[0] = mpc_decoder_bitstream_read(d, 1);
    } else {
        d->MS_Flag[0] = 0;
    }

    // consecutive subbands
    ++ResL; ++ResR; // increase pointers
    for (n=1; n <= d->Max_Band; ++n, ++ResL, ++ResR)
    {
        idx   = mpc_decoder_huffman_decode_fast(d, mpc_table_HuffHdr);
        *ResL = (idx!=4) ? *(ResL-1) + idx : (int) mpc_decoder_bitstream_read(d, 4);

        idx   = mpc_decoder_huffman_decode_fast(d, mpc_table_HuffHdr);
        *ResR = (idx!=4) ? *(ResR-1) + idx : (int) mpc_decoder_bitstream_read(d, 4);

        if (d->MS_used && !(*ResL==0 && *ResR==0)) {
            d->MS_Flag[n] = mpc_decoder_bitstream_read(d, 1);
        }

        // only perform following procedures up to the maximum non-zero subband
        if (*ResL!=0 || *ResR!=0) {
            Max_used_Band = n;
        } else {
            d->MS_Flag[n] = 0;
        }
    }
    /****************************** SCFI ******************************/
    L     = d->SCFI_L;
    R     = d->SCFI_R;
    ResL  = d->Res_L;
    ResR  = d->Res_R;
    for (n=0; n <= Max_used_Band; ++n, ++L, ++R, ++ResL, ++ResR) {
        if (*ResL) *L = mpc_decoder_huffman_decode_faster(d, mpc_table_HuffSCFI);
        if (*ResR) *R = mpc_decoder_huffman_decode_faster(d, mpc_table_HuffSCFI);
    }

    /**************************** SCF/DSCF ****************************/
    ResL  = d->Res_L;
    ResR  = d->Res_R;
    L     = d->SCF_Index_L[0];
    R     = d->SCF_Index_R[0];
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR, L+=3, R+=3) {
        if (*ResL)
        {
            switch (d->SCFI_L[n])
            {
            case 1:
                idx = Decode_DSCF ();
                L[0] = (idx!=8) ? SCF_DIFF(L[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                idx = Decode_DSCF ();
                L[1] = (idx!=8) ? SCF_DIFF(L[0], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                L[2] = L[1];
                break;
            case 3:
                idx = Decode_DSCF ();
                L[0] = (idx!=8) ? SCF_DIFF(L[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                L[1] = L[0];
                L[2] = L[1];
                break;
            case 2:
                idx = Decode_DSCF ();
                L[0] = (idx!=8) ? SCF_DIFF(L[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                L[1] = L[0];
                idx = Decode_DSCF ();
                L[2] = (idx!=8) ? SCF_DIFF(L[1], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            case 0:
                idx = Decode_DSCF ();
                L[0] = (idx!=8) ? SCF_DIFF(L[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                idx = Decode_DSCF ();
                L[1] = (idx!=8) ? SCF_DIFF(L[0], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                idx = Decode_DSCF ();
                L[2] = (idx!=8) ? SCF_DIFF(L[1], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            default:
                return;
                break;
            }
        }
        if (*ResR)
        {
            switch (d->SCFI_R[n])
            {
            case 1:
                idx = Decode_DSCF ();
                R[0] = (idx!=8) ? SCF_DIFF(R[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                idx = Decode_DSCF ();
                R[1] = (idx!=8) ? SCF_DIFF(R[0], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                R[2] = R[1];
                break;
            case 3:
                idx = Decode_DSCF ();
                R[0] = (idx!=8) ? SCF_DIFF(R[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                R[1] = R[0];
                R[2] = R[1];
                break;
            case 2:
                idx = Decode_DSCF ();
                R[0] = (idx!=8) ? SCF_DIFF(R[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                R[1] = R[0];
                idx = Decode_DSCF ();
                R[2] = (idx!=8) ? SCF_DIFF(R[1], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            case 0:
                idx = Decode_DSCF ();
                R[0] = (idx!=8) ? SCF_DIFF(R[2], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                idx = Decode_DSCF ();
                R[1] = (idx!=8) ? SCF_DIFF(R[0], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                idx = Decode_DSCF ();
                R[2] = (idx!=8) ? SCF_DIFF(R[1], idx) : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            default:
                return;
                break;
            }
        }
    }

    if (fastSeeking)
        return;

    /***************************** Samples ****************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    LQ    = d->Q[0].L;
    RQ    = d->Q[0].R;
    for (n=0; n <= Max_used_Band; ++n, ++ResL, ++ResR, LQ+=36, RQ+=36)
    {
        /************** links **************/
        switch (*ResL)
        {
        case  -2: case  -3: case  -4: case  -5: case  -6: case  -7: case  -8: case  -9:
        case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17:
            LQ += 36;
            break;
        case -1:
            for (k=0; k<36; k++ ) {
                tmp  = mpc_random_int(d);
                *LQ++ = ((tmp >> 24) & 0xFF) + ((tmp >> 16) & 0xFF) + ((tmp >>  8) & 0xFF) + ((tmp >>  0) & 0xFF) - 510;
            }
            break;
        case 0:
            LQ += 36;// increase pointer
            break;
        case 1:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][1];
                LUT = LUT1_1;
                max_length = 9;
            } else {
                Table = mpc_table_HuffQ[0][1];
                LUT = LUT1_0;
                max_length = 6;
            }
            for (k=0; k<12; ++k)
            {
                idx  = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                *LQ++ = idx30[idx];
                *LQ++ = idx31[idx];
                *LQ++ = idx32[idx];
            }
            break;
        case 2:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][2];
                LUT = LUT2_1;
                max_length = 10;
            } else {
                Table = mpc_table_HuffQ[0][2];
                LUT = LUT2_0;
                max_length = 7;
            }
            for (k=0; k<18; ++k)
            {
                idx  = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                *LQ++ = idx50[idx];
                *LQ++ = idx51[idx];
            }
            break;
        case 3:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][3];
                LUT = LUT3_1;
                max_length = 5;
            } else {
                Table = mpc_table_HuffQ[0][3];
                LUT = LUT3_0;
                max_length = 4;
            }
            for (k=0; k<36; ++k)
                *LQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            break;
        case 4:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][4];
                LUT = LUT4_1;
                max_length = 5;
            } else {
                Table = mpc_table_HuffQ[0][4];
                LUT = LUT4_0;
                max_length = 4;
            }
            for (k=0; k<36; ++k)
                *LQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            break;
        case 5:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][5];
                LUT = LUT5_1;
                max_length = 8;
            } else {
                Table = mpc_table_HuffQ[0][5];
                LUT = LUT5_0;
                max_length = 6;
            }
            for (k=0; k<36; ++k)
                *LQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            break;
        case 6:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][6];
                LUT = LUT6_1;
                max_length = 7;
                for (k=0; k<36; ++k)
                    *LQ++ = HUFFMAN_DECODE_FASTERER ( d, Table, LUT, max_length );
            } else {
                Table = mpc_table_HuffQ[0][6];
                LUT = LUT6_0;
                max_length = 7;
                for (k=0; k<36; ++k)
                    *LQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            }
            break;
        case 7:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][7];
                LUT = LUT7_1;
                max_length = 8;
                for (k=0; k<36; ++k)
                    *LQ++ = HUFFMAN_DECODE_FASTERER ( d, Table, LUT, max_length );
            } else {
                Table = mpc_table_HuffQ[0][7];
                LUT = LUT7_0;
                max_length = 8;
                for (k=0; k<36; ++k)
                    *LQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            }           
            break;
        case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
            tmp = Dc[*ResL];
            for (k=0; k<36; ++k)
                *LQ++ = (mpc_int16_t)mpc_decoder_bitstream_read(d, Res_bit[*ResL]) - tmp;
            break;
        default:
            return;
        }
        /************** rechts **************/
        switch (*ResR)
        {
        case  -2: case  -3: case  -4: case  -5: case  -6: case  -7: case  -8: case  -9:
        case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17:
            RQ += 36;
            break;
        case -1:
                for (k=0; k<36; k++ ) {
                    tmp  = mpc_random_int(d);
                    *RQ++ = ((tmp >> 24) & 0xFF) + ((tmp >> 16) & 0xFF) + ((tmp >>  8) & 0xFF) + ((tmp >>  0) & 0xFF) - 510;
                }
                break;
            case 0:
                RQ += 36;// increase pointer
                break;
            case 1:
                if (mpc_decoder_bitstream_read(d, 1)) {
                    Table = mpc_table_HuffQ[1][1];
                    LUT = LUT1_1;
                    max_length = 9;
                } else {
                    Table = mpc_table_HuffQ[0][1];
                    LUT = LUT1_0;
                    max_length = 6;
                }
                for (k=0; k<12; ++k)
                {
                    idx = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                    *RQ++ = idx30[idx];
                    *RQ++ = idx31[idx];
                    *RQ++ = idx32[idx];
                }
                break;
            case 2:
                if (mpc_decoder_bitstream_read(d, 1)) {
                    Table = mpc_table_HuffQ[1][2];
                    LUT = LUT2_1;
                    max_length = 10;
                } else {
                    Table = mpc_table_HuffQ[0][2];
                    LUT = LUT2_0;
                    max_length = 7;
                }
                for (k=0; k<18; ++k)
                {
                    idx = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                    *RQ++ = idx50[idx];
                    *RQ++ = idx51[idx]; 
                }
                break;
            case 3:
                if (mpc_decoder_bitstream_read(d, 1)) {
                    Table = mpc_table_HuffQ[1][3];
                    LUT = LUT3_1;
                    max_length = 5;
                } else {
                    Table = mpc_table_HuffQ[0][3];
                    LUT = LUT3_0;
                    max_length = 4;
                }
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                break;
            case 4:
                if (mpc_decoder_bitstream_read(d, 1)) {
                    Table = mpc_table_HuffQ[1][4];
                    LUT = LUT4_1;
                    max_length = 5;
                } else {
                    Table = mpc_table_HuffQ[0][4];
                    LUT = LUT4_0;
                    max_length = 4;
                }
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                break;
            case 5:
                if (mpc_decoder_bitstream_read(d, 1)) {
                    Table = mpc_table_HuffQ[1][5];
                    LUT = LUT5_1;
                    max_length = 8;
                } else {
                    Table = mpc_table_HuffQ[0][5];
                    LUT = LUT5_0;
                    max_length = 6;
                }
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
                break;
            case 6:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][6];
                LUT = LUT6_1;
                max_length = 7;
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTERER ( d, Table, LUT, max_length );
            } else {
                Table = mpc_table_HuffQ[0][6];
                LUT = LUT6_0;
                max_length = 7;
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            }
            break;
        case 7:
            if (mpc_decoder_bitstream_read(d, 1)) {
                Table = mpc_table_HuffQ[1][7];
                LUT = LUT7_1;
                max_length = 8;
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTERER ( d, Table, LUT, max_length );
            } else {
                Table = mpc_table_HuffQ[0][7];
                LUT = LUT7_0;
                max_length = 8;
                for (k=0; k<36; ++k)
                    *RQ++ = HUFFMAN_DECODE_FASTEST ( d, Table, LUT, max_length );
            }           
            break;
            case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
                tmp = Dc[*ResR];
                for (k=0; k<36; ++k)
                    *RQ++ = (mpc_int16_t)mpc_decoder_bitstream_read(d, Res_bit[*ResR]) - tmp;
                break;
            default:
                return;
        }
    }
}

void mpc_decoder_setup(mpc_decoder *d, mpc_reader *r)
{
  d->r = r;

  d->MPCHeaderPos = 0;
  d->StreamVersion = 0;
  d->MS_used = 0;
  d->FwdJumpInfo = 0;
  d->ActDecodePos = 0;
  d->FrameWasValid = 0;
  d->OverallFrames = 0;
  d->DecodedFrames = 0;
  d->MaxDecodedFrames = 0;
  d->TrueGaplessPresent = 0;
  d->last_block_samples = 0;
  d->WordsRead = 0;
  d->Max_Band = 0;
  d->SampleRate = 0;
  d->__r1 = 1;
  d->__r2 = 1;

  d->dword = 0;
  d->pos = 0;
  d->Zaehler = 0;
  d->Ring = 0;
  d->WordsRead = 0;
  d->Max_Band = 0;
  d->SeekTable_Step = 0;
  d->SeekTable_Mask = 0;
  d->SeekTableCounter = 0;

  mpc_decoder_initialisiere_quantisierungstabellen(d, 1.0f);

  LOOKUP ( mpc_table_HuffQ[0][1], 27, LUT1_0  );
  LOOKUP ( mpc_table_HuffQ[1][1], 27, LUT1_1  );
  LOOKUP ( mpc_table_HuffQ[0][2], 25, LUT2_0  );
  LOOKUP ( mpc_table_HuffQ[1][2], 25, LUT2_1  );
  LOOKUP ( mpc_table_HuffQ[0][3], 7,  LUT3_0  );
  LOOKUP ( mpc_table_HuffQ[1][3], 7,  LUT3_1  );
  LOOKUP ( mpc_table_HuffQ[0][4], 9,  LUT4_0  );
  LOOKUP ( mpc_table_HuffQ[1][4], 9,  LUT4_1  );
  LOOKUP ( mpc_table_HuffQ[0][5], 15, LUT5_0  );
  LOOKUP ( mpc_table_HuffQ[1][5], 15, LUT5_1  );
  LOOKUP ( mpc_table_HuffQ[0][6], 31, LUT6_0  );
  LOOKUP ( mpc_table_HuffQ[1][6], 31, LUT6_1  );
  LOOKUP ( mpc_table_HuffQ[0][7], 63, LUT7_0  );
  LOOKUP ( mpc_table_HuffQ[1][7], 63, LUT7_1  );
  LOOKUP ( mpc_table_HuffDSCF,    16, LUTDSCF );

  d->SeekTable = Seekbuffer;
  d->Speicher = Speicher;
  d->Y_L = Y_L;
  d->Y_R = Y_R;

  #if defined(CPU_COLDFIRE)
  coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
  #endif
}

static void mpc_decoder_set_streaminfo(mpc_decoder *d, mpc_streaminfo *si)
{
    mpc_uint32_t seekTableSize;

    mpc_decoder_reset_synthesis(d);
    mpc_decoder_reset_globals(d);

    d->StreamVersion      = si->stream_version;
    d->MS_used            = si->ms;
    d->Max_Band           = si->max_band;
    d->OverallFrames      = si->frames;
    d->MPCHeaderPos       = si->header_position;
    d->TrueGaplessPresent = si->is_true_gapless;
    d->SampleRate         = (mpc_int32_t)si->sample_freq;

    d->samples_to_skip = MPC_DECODER_SYNTH_DELAY;

    memset(d->SeekTable, 0, sizeof(Seekbuffer));

    // limit used table size to MPC_SEEK_BUFFER_SIZE
    seekTableSize = min(si->frames, MPC_SEEK_BUFFER_SIZE);
    // frames per buffer to not exceed buffer and to be able to seek full file
    while ( seekTableSize < si->frames / (1<<d->SeekTable_Step) ) 
    {
        d->SeekTable_Step++;
    }
    d->SeekTable_Mask = (1 << d->SeekTable_Step) - 1;
}

mpc_bool_t mpc_decoder_initialize(mpc_decoder *d, mpc_streaminfo *si) 
{
    mpc_uint32_t bitPos;
    mpc_uint32_t fpos;

    mpc_decoder_set_streaminfo(d, si);

    // setting position to the beginning of the data-bitstream
    bitPos = get_initial_fpos(d, d->StreamVersion);
    fpos = bitPos >> 5;

    // fill buffer and initialize decoder
    f_seek(d, fpos*4 + d->MPCHeaderPos);
    f_read_dword(d, d->Speicher, MEMSIZE);
    d->Ring = 0;
    d->Zaehler = 0;
    d->pos = bitPos & 31;
    d->WordsRead = fpos;
    d->dword = SWAP(d->Speicher[0]);
    d->next = SWAP(d->Speicher[1]);
    
    return TRUE;
}

// jumps over the current frame
mpc_uint32_t mpc_decoder_jump_frame(mpc_decoder *d) {

    mpc_uint32_t frameSize;

    // ensure the buffer is full
    mpc_decoder_update_buffer(d);

    // bits in frame
    frameSize = mpc_decoder_bitstream_read(d, 20);

    // jump forward
    mpc_decoder_seek_forward(d, frameSize);

    return frameSize + 20;

}

static mpc_uint32_t get_initial_fpos(mpc_decoder *d, mpc_uint32_t StreamVersion)
{
    mpc_uint32_t fpos = 0;
    (void) StreamVersion;
    
    // setting position to the beginning of the data-bitstream
    switch ( d->StreamVersion ) {
    case  0x04: fpos =  48; break;
    case  0x05:
    case  0x06: fpos =  64; break;
    case  0x07:
    case  0x17: fpos = 200; break;
    }
    return fpos;
}

mpc_bool_t mpc_decoder_seek_seconds(mpc_decoder *d, double seconds) 
{
    return mpc_decoder_seek_sample(d, (mpc_int64_t)(seconds * (double)d->SampleRate + 0.5));
}

void mpc_decoder_reset_state(mpc_decoder *d) {

    memset(d->Y_L             , 0, sizeof Y_L              );
    memset(d->Y_R             , 0, sizeof Y_R              );
#ifdef SCF_HACK
    memset(d->SCF_Index_L     , -128, sizeof d->SCF_Index_L   );
    memset(d->SCF_Index_R     , -128, sizeof d->SCF_Index_R   );
#else
    memset(d->SCF_Index_L     , 0, sizeof d->SCF_Index_L      );
    memset(d->SCF_Index_R     , 0, sizeof d->SCF_Index_R      );
#endif
    memset(d->Res_L           , 0, sizeof d->Res_L            );
    memset(d->Res_R           , 0, sizeof d->Res_R            );
    memset(d->SCFI_L          , 0, sizeof d->SCFI_L           );
    memset(d->SCFI_R          , 0, sizeof d->SCFI_R           );
#ifdef MPC_SUPPORT_SV456
    memset(d->DSCF_Flag_L     , 0, sizeof d->DSCF_Flag_L      );
    memset(d->DSCF_Flag_R     , 0, sizeof d->DSCF_Flag_R      );
#endif
    memset(d->Q               , 0, sizeof d->Q                );
    memset(d->MS_Flag         , 0, sizeof d->MS_Flag          );

}

mpc_bool_t mpc_decoder_seek_sample(mpc_decoder *d, mpc_int64_t destsample) 
{
    mpc_uint32_t fpos = 0;        // the bit to seek to
    mpc_uint32_t seekFrame = 0;   // the frame to seek to
    mpc_uint32_t lastFrame = 0;   // last frame to seek to before scanning scale factors
    mpc_int32_t  delta = 0;       // direction of seek
    
    destsample += MPC_DECODER_SYNTH_DELAY;
    seekFrame = (mpc_uint32_t) ((destsample) / MPC_FRAME_LENGTH);
    d->samples_to_skip = (mpc_uint32_t)((destsample) % MPC_FRAME_LENGTH);

    // prevent from desired position out of allowed range
    seekFrame = seekFrame < d->OverallFrames  ?  seekFrame  :  d->OverallFrames;

    // seek direction (note: avoids casting to int64)
    delta = (d->DecodedFrames > seekFrame ? -(mpc_int32_t)(d->DecodedFrames - seekFrame) : (mpc_int32_t)(seekFrame - d->DecodedFrames));

    if (seekFrame > SEEK_PRE_DECODE) 
        lastFrame = seekFrame - SEEK_PRE_DECODE + 1 - (1<<d->SeekTable_Step);

    if (d->MaxDecodedFrames == 0) // nothing decoded yet, parse stream
    {
        mpc_decoder_reset_state(d);

        // starts from the beginning since no frames have been decoded yet, or not using seek table
        fpos = get_initial_fpos(d, d->StreamVersion);

        // seek to the first frame
        mpc_decoder_seek_to(d, fpos);

        // jump to the last frame via parsing, updating seek table
        d->SeekTable[0] = (mpc_uint32_t)fpos;
        d->SeekTableCounter = 0;
        for (d->DecodedFrames = 0; d->DecodedFrames < lastFrame; d->DecodedFrames++)
        {
            d->SeekTableCounter += mpc_decoder_jump_frame(d);
            if (0 == ((d->DecodedFrames+1) & (d->SeekTable_Mask)))
            {
                d->SeekTable[(d->DecodedFrames+1)>>d->SeekTable_Step] = d->SeekTableCounter;
                d->MaxDecodedFrames = d->DecodedFrames;
                d->SeekTableCounter = 0;
            }
        }
    } 
    else if (delta < 0) // jump backwards, seek table is already available
    {
        mpc_decoder_reset_state(d);

        // jumps backwards using the seek table
        fpos = d->SeekTable[0];
        for (d->DecodedFrames = 0; d->DecodedFrames < lastFrame; d->DecodedFrames++)
        {
            if (0 == ((d->DecodedFrames+1) & (d->SeekTable_Mask)))
            {
                fpos += d->SeekTable[(d->DecodedFrames+1)>>d->SeekTable_Step];
                d->SeekTableCounter = 0;
            }
        }
        mpc_decoder_seek_to(d, fpos);
    } 
    else if (delta > SEEK_PRE_DECODE) // jump forward, seek table is available
    {
        mpc_decoder_reset_state(d);

        // 1st loop: jump to the last usable position in the seek table
        fpos = mpc_decoder_bits_read(d);
        for (; d->DecodedFrames < d->MaxDecodedFrames && d->DecodedFrames < lastFrame; d->DecodedFrames++)
        {
            if (0 == ((d->DecodedFrames+1) & (d->SeekTable_Mask)))
            {
                fpos += d->SeekTable[(d->DecodedFrames+1)>>d->SeekTable_Step];
                d->SeekTableCounter = 0;
            }
        }
        mpc_decoder_seek_to(d, fpos);
        
        // 2nd loop: jump the residual frames via parsing, update seek table
        for (;d->DecodedFrames < lastFrame; d->DecodedFrames++)
        {
            d->SeekTableCounter += mpc_decoder_jump_frame(d);
            if (0 == ((d->DecodedFrames+1) & (d->SeekTable_Mask)))
            {
                d->SeekTable[(d->DecodedFrames+1)>>d->SeekTable_Step] = d->SeekTableCounter;
                d->MaxDecodedFrames = d->DecodedFrames;
                d->SeekTableCounter = 0;
            }
        }
    }
    // until here we jumped to desired position -SEEK_PRE_DECODE frames

    // now we decode the last SEEK_PRE_DECODE frames until we reach the seek
    // position. this is neccessary as mpc uses entropy coding in time domain
    for (;d->DecodedFrames < seekFrame; d->DecodedFrames++) 
    {
        mpc_uint32_t   FrameBitCnt;

        d->FwdJumpInfo  = mpc_decoder_bitstream_read(d, 20);    // read jump-info
        d->ActDecodePos = (d->Zaehler << 5) + d->pos;
        FrameBitCnt  = mpc_decoder_bits_read(d);  
        // scanning the scalefactors (and check for validity of frame)
        if (d->StreamVersion >= 7)
        {
            mpc_decoder_read_bitstream_sv7(d, (d->DecodedFrames < seekFrame - 1));
        }
        else 
        {
#ifdef MPC_SUPPORT_SV456
            mpc_decoder_read_bitstream_sv6(d);
#else
            return FALSE;
#endif
        }

        FrameBitCnt = mpc_decoder_bits_read(d) - FrameBitCnt;

        if (d->FwdJumpInfo > FrameBitCnt) 
            mpc_decoder_seek_forward(d, d->FwdJumpInfo - FrameBitCnt);
        else if (FrameBitCnt != d->FwdJumpInfo ) 
            // Bug in perform_jump;
            return FALSE;
            
        // update seek table, if there new entries to fill
        d->SeekTableCounter += d->FwdJumpInfo + 20;
        if (0 == ((d->DecodedFrames+1) & (d->SeekTable_Mask)))
        {
            d->SeekTable[(d->DecodedFrames+1)>>d->SeekTable_Step] = d->SeekTableCounter;
            d->MaxDecodedFrames = d->DecodedFrames;
            d->SeekTableCounter = 0;
        }

        // update buffer
        mpc_decoder_update_buffer(d);

        if (d->DecodedFrames == seekFrame - 1) 
        {
            // initialize the synth correctly for perfect decoding
            mpc_decoder_requantisierung(d, d->Max_Band);
            mpc_decoder_synthese_filter_float(d, NULL);
        }
    }

    return TRUE;
}


void mpc_decoder_fill_buffer(mpc_decoder *d) {

    f_read_dword(d, d->Speicher, MEMSIZE);
    d->dword = SWAP(d->Speicher[d->Zaehler = 0]);
    d->next = SWAP(d->Speicher[1]);
    d->Ring = 0;

}


void mpc_decoder_update_buffer(mpc_decoder *d) 
{
    if ((d->Ring ^ d->Zaehler) & MEMSIZE2) {
        // update buffer
        f_read_dword(d, d->Speicher + (d->Ring & MEMSIZE2), MEMSIZE2);
        d->Ring = d->Zaehler;
    }
}


void mpc_decoder_seek_to(mpc_decoder *d, mpc_uint32_t bitPos) {

    // required dword
    mpc_uint32_t fpos = (bitPos >> 5);
    mpc_uint32_t bufferStart = d->WordsRead - d->Zaehler;
    if ((d->Zaehler & MEMSIZE2) != FALSE)
        bufferStart += MEMSIZE2;

    if (fpos >= bufferStart && fpos < bufferStart + MEMSIZE) {

        // required position is within the buffer, no need to seek
        d->Zaehler = (fpos - bufferStart + ((d->Zaehler & MEMSIZE2) != FALSE ? MEMSIZE2 : 0)) & MEMMASK;
        d->pos = bitPos & 31;
        d->WordsRead = fpos;
        d->dword = SWAP(d->Speicher[d->Zaehler]);
        d->next = SWAP(d->Speicher[(d->Zaehler + 1) & MEMMASK]);

        mpc_decoder_update_buffer(d);
        

    } else {

        // DWORD aligned
        f_seek(d, fpos*4 + d->MPCHeaderPos);
        d->Zaehler = 0;
        d->pos = bitPos & 31;
        d->WordsRead = fpos;

        mpc_decoder_fill_buffer(d);

    }
    
}

void mpc_decoder_seek_forward(mpc_decoder *d, mpc_uint32_t bits) {

    bits += d->pos;
    d->pos = bits & 31;
    bits = bits >> 5; // to DWORDs
    d->Zaehler = (d->Zaehler + bits) & MEMMASK;
    d->dword = SWAP(d->Speicher[d->Zaehler]);
    d->next = SWAP(d->Speicher[(d->Zaehler + 1) & MEMMASK]);
    d->WordsRead += bits;

}

