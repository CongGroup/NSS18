#pragma once
#include <cstdint>
#include <string>
#include <openssl/aes.h>

typedef uint8_t byte;
typedef uint8_t block[16];

struct extraData
{
	int reserve;
};

class ORE
{
public:
	
	ORE(byte sizeb, byte sizec = 64);
	~ORE();

	void setupKey(const std::string& strKey);

	int genLeft(std::string& left, const uint32_t msg, const byte cmp, const extraData* data = 0);
	int genRight(std::string& right, const uint32_t msg, const extraData* data = 0);
	bool cmp(const std::string& left, const std::string& right);

	int getLeftSize();
	int getRightSize();

private:
	void setBlock(byte* dst, const uint64_t left, const uint64_t right);
	void setKey(AES_KEY& key, const byte* src, const int size);
	void aes(byte* dst, const byte* src, const AES_KEY& key);
	void prp(byte* dst, const byte* src, int len, const AES_KEY* keyThree,  bool inv = false);
	void prf(byte* dst, const byte* src, const void* key);
	void prg(byte* dst);


	void permuteBlockOrder(int* block_loc, int nblocks);


	byte blockLenBit;
	byte cmpLenBit;

	uint32_t prgCounter[4];
	
	byte masterKey[32];
	AES_KEY trapdoorKey;
	AES_KEY nonceKey;
	AES_KEY prgKey;
	AES_KEY prpKey[3];

	static const int maxBlockCount = 16;

	const byte ORE_EQUAL = 0;
	const byte ORE_SMALL = -1;
	const byte ORE_LARGE = 1;
	
};

