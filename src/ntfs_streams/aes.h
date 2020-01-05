#pragma once

#include <stdint.h>

class Aes128
{
public:
	Aes128(const uint8_t* key16, const uint8_t* init_iv = nullptr);
	void reset_iv(const uint8_t* init_iv = nullptr);
	void encrypt(const uint8_t* in_bytes16, uint8_t* out_bytes16);
	void decrypt(const uint8_t* in_bytes16, uint8_t* out_bytes16);
protected:
	uint8_t iv[16]; // initializing vector
	uint32_t w1[44]; // reversed expanded key
};

