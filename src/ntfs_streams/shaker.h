#pragma once
#include <inttypes.h>

// shakes (shuffles) 32-byte blocks
class Shaker
{
public:
	Shaker(const uint8_t *key20);
	void encrypt(const uint8_t *in32, uint8_t *out32); // can encode in-place
	void decrypt(const uint8_t *in32, uint8_t *out32); // can decode in-place
protected:
	int enc_seq[32];
	int dec_seq[32];
};

