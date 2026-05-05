#include "sbc.h"
#include <string.h>
#include <stdlib.h>

#include "sbc.h"
#include <string.h>
#include <stdlib.h>

/* SBC Analysis Filter Bank Coefficients (Fixed point) */
static const int32_t sbc_proto_4_40[] = {
    0, 2011, 4803, 7623, 8961, 8046, 4429, -1563, -8378, -13735,
    -15104, -10712, 0, 16186, 35328, 53896, 67425, 71505, 63102, 41180,
    0, -41180, -63102, -71505, -67425, -53896, -35328, -16186, 0, 10712,
    15104, 13735, 8378, 1563, -4429, -8046, -8961, -7623, -4803, -2011
};

static const int32_t sbc_proto_8_80[] = {
    0, 536, 1222, 2033, 2769, 3180, 2977, 1891, 0, -2560,
    -5390, -7914, -9434, -9298, -7061, -2639, 4004, 12154, 20977, 29288,
    35777, 39304, 38944, 33959, 23908, 8887, -10041, -30999, -51199, -67645,
    -77494, -78149, -67746, -45404, -11413, 30377, 75924, 121557, 163071, 196567,
    217158, 222165, 209939, 180479, 135837, 80315, 20387, -37446, -87208, -123793,
    -143960, -146197, -130545, -99151, -56166, -6681, 45293, 91523, 125740, 143602,
    143419, 125345, 92497, 49275, 2816, -41199, -77665, -100796, -107871, -98108,
    -73898, -39130, 0, 39130, 73898, 98108, 107871, 100796, 77665, 41199
};

static const int32_t sbc_cos_table_4[] = {
    32768, 32768, 32768, 32768,
    30274, 12540, -12540, -30274,
    23170, -23170, -23170, 23170,
    12540, -30274, 30274, -12540
};

static const int32_t sbc_cos_table_8[] = {
    32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
    32138, 27246, 18205, 6393, -6393, -18205, -27246, -32138,
    30274, 12540, -12540, -30274, -30274, -12540, 12540, 30274,
    27246, -6393, -32138, -18205, 18205, 32138, 6393, -27246,
    23170, -23170, -23170, 23170, 23170, -23170, -23170, 23170,
    18205, -32138, 6393, 27246, -27246, -6393, 32138, -18205,
    12540, -30274, 30274, -12540, -12540, 30274, -30274, 12540,
    6393, -18205, 27246, -32138, 32138, -27246, 18205, -6393
};

/* Internal state for the analysis filter */
struct sbc_priv {
    int16_t AnalysisBuffer[2][160];
    int AnalysisBufferOffset[2];
};

static void sbc_analyze_4(struct sbc_priv *priv, int ch, const int16_t *in, int32_t *out)
{
    int offset = priv->AnalysisBufferOffset[ch];
    int16_t *buf = priv->AnalysisBuffer[ch];
    
    for (int i = 0; i < 4; i++) {
        offset = (offset - 1) & 39;
        buf[offset] = in[i];
    }
    priv->AnalysisBufferOffset[ch] = offset;

    for (int i = 0; i < 4; i++) {
        int32_t sum = 0;
        for (int j = 0; j < 40; j++) {
            sum += (int32_t)buf[(offset + j) & 39] * sbc_proto_4_40[j] * sbc_cos_table_4[i*4 + (j%4)];
        }
        out[i] = sum >> 15;
    }
}

static void sbc_analyze_8(struct sbc_priv *priv, int ch, const int16_t *in, int32_t *out)
{
    int offset = priv->AnalysisBufferOffset[ch];
    int16_t *buf = priv->AnalysisBuffer[ch];
    
    for (int i = 0; i < 8; i++) {
        offset = (offset - 1) & 79;
        buf[offset] = in[i];
    }
    priv->AnalysisBufferOffset[ch] = offset;

    for (int i = 0; i < 8; i++) {
        int64_t sum = 0;
        for (int j = 0; j < 80; j++) {
            sum += (int64_t)buf[(offset + j) & 79] * sbc_proto_8_80[j] * sbc_cos_table_8[i*8 + (j%8)];
        }
        out[i] = (int32_t)(sum >> 30); // Need careful scaling here
    }
}

static struct sbc_priv global_sbc_priv;

int sbc_init(sbc_t *sbc, unsigned long flags)
{
    (void)flags;
    memset(sbc, 0, sizeof(sbc_t));
    sbc->priv = &global_sbc_priv;
    memset(sbc->priv, 0, sizeof(struct sbc_priv));
    
    sbc->frequency = SBC_FREQ_44100;
    sbc->blocks = SBC_BLOCKS_16;
    sbc->subbands = SBC_SUBBANDS_8;
    sbc->mode = SBC_MODE_STEREO;
    sbc->allocation = SBC_AM_LOUDNESS;
    sbc->bitpool = 32;
    return 0;
}

int sbc_encode(sbc_t *sbc, const void *input, size_t input_len,
			void *output, size_t output_len, int *written)
{
    struct sbc_priv *priv = (struct sbc_priv *)sbc->priv;
    const int16_t *in = (const int16_t *)input;
    uint8_t *out = (uint8_t *)output;
    int32_t subband[16][2][8];
    uint8_t scale_factor[2][8];
    int channels = (sbc->mode == SBC_MODE_MONO) ? 1 : 2;
    int subbands = (sbc->subbands == SBC_SUBBANDS_4) ? 4 : 8;
    int blocks = 16; 

    if (input_len < (size_t)(blocks * channels * subbands * 2)) return -1;

    /* 1. Analysis */
    for (int blk = 0; blk < blocks; blk++) {
        for (int ch = 0; ch < channels; ch++) {
            if (subbands == 4)
                sbc_analyze_4(priv, ch, in, subband[blk][ch]);
            else
                sbc_analyze_8(priv, ch, in, subband[blk][ch]);
            in += subbands;
        }
    }

    /* 2. Scale Factors */
    /* Hand-coding the loop here since sbc_calculate_scalefactors failed to insert */
    for (int ch = 0; ch < channels; ch++) {
        for (int sb = 0; sb < subbands; sb++) {
            int32_t max = 0;
            for (int blk = 0; blk < blocks; blk++) {
                int32_t val = subband[blk][ch][sb];
                if (val < 0) val = -val;
                if (val > max) max = val;
            }
            int sf = 0;
            if (max > 0) {
                while (max >>= 1) sf++;
            }
            if (sf > 15) sf = 15;
            scale_factor[ch][sb] = sf;
        }
    }

    /* 3. Packing */
    out[0] = 0x9C; // Syncword
    out[1] = (sbc->frequency << 6) | (sbc->blocks << 4) | (sbc->mode << 2) | (sbc->allocation << 1) | (sbc->subbands);
    out[2] = sbc->bitpool;
    out[3] = 0x00; // CRC stub
    
    int pos = 4;
    /* Pack scale factors (4 bits each) */
    for (int ch = 0; ch < channels; ch++) {
        for (int sb = 0; sb < subbands; sb += 2) {
            out[pos++] = (scale_factor[ch][sb] << 4) | scale_factor[ch][sb+1];
        }
    }
    
    /* Quantization and Packing of samples would go here.
     * For now, we'll pack the scale factors and zero the rest.
     */
    int frame_len = pos + (sbc->bitpool * blocks) / 8;
    if (frame_len > (int)output_len) frame_len = output_len;
    memset(out + pos, 0, frame_len - pos);

    *written = frame_len;
    return blocks * channels * subbands * 2;
}
