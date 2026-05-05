#ifndef SBC_H
#define SBC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SBC_FREQ_16000		0x00
#define SBC_FREQ_32000		0x01
#define SBC_FREQ_44100		0x02
#define SBC_FREQ_48000		0x03

#define SBC_BLOCKS_4		0x00
#define SBC_BLOCKS_8		0x01
#define SBC_BLOCKS_12		0x02
#define SBC_BLOCKS_16		0x03

#define SBC_MODE_MONO		0x00
#define SBC_MODE_DUAL_CHANNEL	0x01
#define SBC_MODE_STEREO		0x02
#define SBC_MODE_JOINT_STEREO	0x03

#define SBC_AM_LOUDNESS		0x00
#define SBC_AM_SNR		0x01

#define SBC_SUBBANDS_4		0x00
#define SBC_SUBBANDS_8		0x01

struct sbc_struct {
	uint32_t flags;
	uint8_t frequency;
	uint8_t blocks;
	uint8_t subbands;
	uint8_t mode;
	uint8_t allocation;
	uint8_t bitpool;
	uint8_t endian;

	void *priv;
};

typedef struct sbc_struct sbc_t;

int sbc_init(sbc_t *sbc, unsigned long flags);
int sbc_reinit(sbc_t *sbc, unsigned long flags);
int sbc_encode(sbc_t *sbc, const void *input, size_t input_len,
			void *output, size_t output_len, int *written);
void sbc_finish(sbc_t *sbc);

#endif /* SBC_H */
