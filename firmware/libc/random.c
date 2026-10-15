/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/*
   Adapted to Rockbox by Skye Green
*/

#include "tick.h"
#include "general.h"
#include <limits.h>
#include <stdint.h>

/* This is xoroshiro64** 1.0, our 32-bit all-purpose, rock-solid,
   small-state generator. It is extremely fast and it passes all tests we
   are aware of, but its state space is not large enough for any parallel
   application.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoroshiro64* is even faster.

   The state must be seeded so that it is not everywhere zero. */


static inline uint32_t rotl(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}

// Initialize with a default seed.
// Constant is the first 16 digits of fractional part of pi
static uint32_t s[2] = {
	0x243F6A88U,
	0x85A308D3U,
};

static unsigned long next_reseed_tick = 0;
#define RESEED_PERIOD HZ*60*15

/* seeds the RNG using a single run of SplitMix64 */
void srand(unsigned int seed) {
	/* This is a fixed-increment version of Java 8's SplittableRandom generator
		See http://dx.doi.org/10.1145/2714064.2660195 and
		http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

		It is a very fast generator passing BigCrush, and it can be useful if
		for some reason you absolutely want 64 bits of state. */
	uint64_t z = (((uint64_t) seed) + 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	z = z ^ (z >> 31);
	s[0] = z & 0xFFFFFFFF;
	s[1] = (z >> 32) & 0xFFFFFFFF;

	next_reseed_tick = current_tick + RESEED_PERIOD;
}


/* generates a random number on [0,RAND_MAX]-interval */
int rand(void) {
	if (TIME_AFTER(current_tick, next_reseed_tick)) {
		next_reseed_tick = current_tick + RESEED_PERIOD;
		srand(prng_newseed());
	}
	const uint32_t s0 = s[0];
	uint32_t s1 = s[1];
	const uint32_t result = rotl(s0 * 0x9E3779BB, 5) * 5;

	s1 ^= s0;
	s[0] = rotl(s0, 26) ^ s1 ^ (s1 << 9); // a, b
	s[1] = rotl(s1, 13); // c

	return result & INT_MAX;
}
