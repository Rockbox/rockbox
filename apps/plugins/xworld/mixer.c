/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "mixer.h"
#include "serializer.h"
#include "sys.h"


static int8_t addclamp(int a, int b) {
	int add = a + b;
	if (add < -128) {
		add = -128;
	}
	else if (add > 127) {
		add = 127;
	}
	return (int8_t)add;
}

mixer_Mixer(struct Mixer* mx, System *stub)
	: sys(stub) {
}

void mixer_init(struct Mixer* mx) {
	memset(_channels, 0, sizeof(_channels));
	mx->_mutex = sys->createMutex();
	sys->startAudio(mixer_mixCallback, this);
}

void mixer_free(struct Mixer* mx) {
	mixer_stopAll();
	sys->stopAudio();
	sys->destroyMutex(mx->_mutex);
}

void mixer_playChannel(struct Mixer* mx, uint8_t channel, const MixerChunk *mc, uint16_t freq, uint8_t volume) {
	debug(DBG_SND, "mixer_playChannel(%d, %d, %d)", channel, freq, volume);
	assert(channel < AUDIO_NUM_CHANNELS);

	// The mutex is acquired in the constructor
	MutexStack(sys, mx->_mutex);
	MixerChannel *ch = &_channels[channel];
	ch->active = true;
	ch->volume = volume;
	ch->chunk = *mc;
	ch->chunkPos = 0;
	ch->chunkInc = (freq << 8) / sys->getOutputSampleRate();

	//At the end of the scope the MutexStack destructor is called and the mutex is released.
}

void mixer_stopChannel(struct Mixer* mx, uint8_t channel) {
	debug(DBG_SND, "mixer_stopChannel(%d)", channel);
	assert(channel < AUDIO_NUM_CHANNELS);
	MutexStack(sys, mx->_mutex);
	_channels[channel].active = false;
}

void mixer_setChannelVolume(struct Mixer* mx, uint8_t channel, uint8_t volume) {
	debug(DBG_SND, "mixer_setChannelVolume(%d, %d)", channel, volume);
	assert(channel < AUDIO_NUM_CHANNELS);
	MutexStack(sys, mx->_mutex);
	_channels[channel].volume = volume;
}

void mixer_stopAll(struct Mixer* mx) {
	debug(DBG_SND, "mixer_stopAll()");
	MutexStack(sys, mx->_mutex);
	for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		_channels[i].active = false;
	}
}

// This is SDL callback. Called in order to populate the buf with len bytes.
// The mixer iterates through all active channels and combine all sounds.

// Since there is no way to know when SDL will ask for a buffer fill, we need
// to synchronize with a mutex so the channels remain stable during the execution
// of this method.
void mixer_mix(struct Mixer* mx, int8_t *buf, int len) {
	int8_t *pBuf;

	MutexStack(sys, mx->_mutex);

	//Clear the buffer since nothing garanty we are receiving clean memory.
	memset(buf, 0, len);

	for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		if (!ch->active)
			continue;

		pBuf = buf;
		for (int j = 0; j < len; ++j, ++pBuf) {

			uint16_t p1, p2;
			uint16_t ilc = (ch->chunkPos & 0xFF);
			p1 = ch->chunkPos >> 8;
			ch->chunkPos += ch->chunkInc;

			if (ch->chunk.loopLen != 0) {
				if (p1 == ch->chunk.loopPos + ch->chunk.loopLen - 1) {
					debug(DBG_SND, "Looping sample on channel %d", i);
					ch->chunkPos = p2 = ch->chunk.loopPos;
				} else {
					p2 = p1 + 1;
				}
			} else {
				if (p1 == ch->chunk.len - 1) {
					debug(DBG_SND, "Stopping sample on channel %d", i);
					ch->active = false;
					break;
				} else {
					p2 = p1 + 1;
				}
			}
			// interpolate
			int8_t b1 = *(int8_t *)(ch->chunk.data + p1);
			int8_t b2 = *(int8_t *)(ch->chunk.data + p2);
			int8_t b = (int8_t)((b1 * (0xFF - ilc) + b2 * ilc) >> 8);

			// set volume and clamp
			*pBuf = addclamp(*pBuf, (int)b * ch->volume / 0x40);  //0x40=64
		}

	}

	// Convert signed 8-bit PCM to unsigned 8-bit PCM. The
	// current version of SDL hangs when using signed 8-bit
	// PCM in combination with the PulseAudio driver.
	pBuf = buf;
	for (int j = 0; j < len; ++j, ++pBuf) {
		*(uint8_t *)pBuf = (*pBuf + 128);
	}
}

void mixer_mixCallback(struct Mixer* mx, void *param, uint8_t *buf, int len) {
	((Mixer *)param)->mix((int8_t *)buf, len);
}

void mixer_saveOrLoad(struct Mixer* mx, Serializer &ser) {
	sys->lockMutex(mx->_mutex);
	for (int i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		Serializer::Entry entries[] = {
			SE_INT(&ch->active, Serializer::SES_BOOL, VER(2)),
			SE_INT(&ch->volume, Serializer::SES_INT8, VER(2)),
			SE_INT(&ch->chunkPos, Serializer::SES_INT32, VER(2)),
			SE_INT(&ch->chunkInc, Serializer::SES_INT32, VER(2)),
			SE_PTR(&ch->chunk.data, VER(2)),
			SE_INT(&ch->chunk.len, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopPos, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopLen, Serializer::SES_INT16, VER(2)),
			SE_END()
		};
		ser.saveOrLoadEntries(entries);
	}
	sys->unlockMutex(mx->_mutex);
};
