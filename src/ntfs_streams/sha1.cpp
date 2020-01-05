#include "pch.h"
#include "sha1.h"

// implmentation is taken from
// https://tools.ietf.org/html/rfc3174


class SHA1Context
{ // usage: SHA1Input (can be called several times) -> SHA1Result (only once)
public:
	static const int SHA1HashSize = 20;
	using Digest = std::array<uint8_t, SHA1HashSize>;

	void SHA1Input(const void* message, unsigned int length);
	Digest SHA1Result();

protected:
	void PadMessage();
	void ProcessMessageBlock();

	// current digest
	uint32_t Intermediate_Hash[SHA1HashSize / 4] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0, };
	uint64_t Length = 0;           // Message length in bits
	int_least16_t Message_Block_Index = 0; // Index into message block array
	uint8_t       Message_Block[64];      // 512-bit message blocks
};



SHA1Context::Digest SHA1Context::SHA1Result()
{
	PadMessage();
	std::fill(std::begin(Message_Block), std::end(Message_Block), 0xCC); // clear potentially private info
	Length = 0;    // and clear length

	Digest digest;
	for (int i = 0; i < SHA1HashSize; ++i)
		digest[i] = Intermediate_Hash[i / 4] >> 8 * (3 - (i & 3));

	return digest;
}

void SHA1Context::SHA1Input(const void* message, unsigned int length)
{
	const uint8_t* ptr = (const uint8_t*)message;
	const uint8_t* ptrE = ptr + length;

	Length += 8 * length;
	for (; ptr < ptrE; ++ptr)
	{
		Message_Block[Message_Block_Index++] = *ptr;
		if (Message_Block_Index == 64)
			ProcessMessageBlock();
	}
}

void SHA1Context::ProcessMessageBlock()
{
	// Constants defined in SHA-1 
	const uint32_t K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

	auto SHA1CircularShift = [](uint32_t bits, uint32_t word)
	{
		return (((word) << (bits)) | ((word) >> (32 - (bits))));
	};

	uint32_t W[80];             // Word sequence
	uint32_t A, B, C, D, E;     // Word buffers

	//  Initialize the first 16 words in the array W
	for (int t = 0; t < 16; t++)
	{
		W[t] = Message_Block[t * 4] << 24;
		W[t] |= Message_Block[t * 4 + 1] << 16;
		W[t] |= Message_Block[t * 4 + 2] << 8;
		W[t] |= Message_Block[t * 4 + 3];
	}

	for (int t = 16; t < 80; t++)
	{
		W[t] = SHA1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
	}

	A = Intermediate_Hash[0];
	B = Intermediate_Hash[1];
	C = Intermediate_Hash[2];
	D = Intermediate_Hash[3];
	E = Intermediate_Hash[4];

	for (int t = 0; t < 20; t++)
	{
		uint32_t temp = SHA1CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (int t = 20; t < 40; t++)
	{
		uint32_t temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (int t = 40; t < 60; t++)
	{
		uint32_t temp = SHA1CircularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (int t = 60; t < 80; t++)
	{
		uint32_t temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = temp;
	}

	Intermediate_Hash[0] += A;
	Intermediate_Hash[1] += B;
	Intermediate_Hash[2] += C;
	Intermediate_Hash[3] += D;
	Intermediate_Hash[4] += E;

	Message_Block_Index = 0;
}

void SHA1Context::PadMessage()
{
	//  Check to see if the current message block is too small to hold
	//  the initial padding bits and length.  If so, we will pad the
	//  block, process it, and then continue padding into a second
	//  block.
	if (Message_Block_Index > 55)
	{
		Message_Block[Message_Block_Index++] = 0x80;
		while (Message_Block_Index < 64)
			Message_Block[Message_Block_Index++] = 0;

		ProcessMessageBlock(); // sets Message_Block_Index to 0
	}
	else
		Message_Block[Message_Block_Index++] = 0x80;

	while (Message_Block_Index < 56)
	{
		Message_Block[Message_Block_Index++] = 0;
	}

	//  Store the message length as the last 8 octets - big endian
	uint64_t len = Length;
	for (int i = 63; i >= 56; --i, len >>= 8)
		Message_Block[i] = (uint8_t)len;

	ProcessMessageBlock();
}


std::array<uint8_t, 20> sha1_digest(const void* message, unsigned int length)
{
	SHA1Context context;
	context.SHA1Input(message, length);
	return context.SHA1Result();
}
