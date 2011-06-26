/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 */

# ifndef LIBMAD_SYNTH_H
# define LIBMAD_SYNTH_H

# include "fixed.h"
# include "frame.h"

struct mad_pcm {
  unsigned int samplerate;              /* sampling frequency (Hz) */
  unsigned short channels;              /* number of channels */
  unsigned short length;                /* number of samples per channel */
  mad_fixed_t samples[2][1152] MEM_ALIGN_ATTR;         /* PCM output samples [ch][sample] */
};

struct mad_synth {
  mad_fixed_t filter[2][2][2][16][8] MEM_ALIGN_ATTR;   /* polyphase filterbank outputs */
                                        /* [ch][eo][peo][s][v] */

  unsigned int phase;                   /* current processing phase */

  struct mad_pcm pcm;                   /* PCM output */
};

void mad_synth_init(struct mad_synth *);

# define mad_synth_finish(synth)  /* nothing */

void mad_synth_mute(struct mad_synth *);

void mad_synth_frame(struct mad_synth *, struct mad_frame const *);

# endif
