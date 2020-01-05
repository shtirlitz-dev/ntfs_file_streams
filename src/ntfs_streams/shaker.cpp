#include "pch.h"
#include "shaker.h"

#include <utility>

Shaker::Shaker(const uint8_t* key20)
{
	for (int i = 0; i < 32; ++i)
		enc_seq[i] = i;


	// shuffle
	uint8_t k; // current key byte
	int bits = 0; // remaining bits in k;
	for (int i = 0; i < 32; ++i) {
		int n = 0;
		if (bits >= 5) {
			n = k;
			k >>= 5;
			bits -= 5;
		}
		else {
			n = bits ? k : 0;
			k = *key20++;
			n |= (k << bits);
			k >>= (5 - bits);
			bits += 3;
		}
		n &= 0x1F;
		if (n != i)
			std::swap(enc_seq[i], enc_seq[n]);
	}
	// reverse
	for (int i = 0; i < 32; ++i)
		dec_seq[enc_seq[i]] = i;
}

void Shaker::encrypt(const uint8_t *in32, uint8_t *out32) // can encode in-place
{
	uint8_t bfr[32];
	memcpy(bfr, in32, 32);
	for (int i = 0; i < 32; ++i)
		out32[i] = bfr[enc_seq[i]];
}

void Shaker::decrypt(const uint8_t *in32, uint8_t *out32) // can decode in-place
{
	uint8_t bfr[32];
	memcpy(bfr, in32, 32);
	for (int i = 0; i < 32; ++i)
		out32[i] = bfr[dec_seq[i]];
}
