#include "ore.h"
#include <stdio.h>
#include <openssl/aes.h>
#include <cstring>

#define DEBUG_INFO 0
using std::string;

ORE::ORE(byte sizeb, byte sizec)
{
	blockLenBit = sizeb;
	cmpLenBit = sizec;
	if (blockLenBit != 2 && blockLenBit != 4 && blockLenBit != 8 && blockLenBit != 16 || cmpLenBit != 64)
	{
		printf("blockLenBit:%d, cmpLenBit:%d\n", blockLenBit, cmpLenBit);
	}
}

ORE::~ORE()
{
}

int ORE::genLeft(std::string & left,  const uint32_t msg, const byte cmp, const extraData* data)
{
	const uint32_t blockCount = sizeof(msg) * 8 / blockLenBit;
	const uint32_t leftBlockLen = AES_BLOCK_SIZE;
	const uint32_t leftSize = blockCount * leftBlockLen;

	left.assign(leftSize, 0);
	byte* leftBase = (byte*)left.data();

	int block_loc[maxBlockCount] = { 0 };
	permuteBlockOrder(block_loc, blockCount);

	if (DEBUG_INFO)
	{
		printf("blockCount:%d\n", blockCount);
		printf("blkLoc enc: ");
		for (int i = 0; i < blockCount; i++)
		{
			printf("%d ", block_loc[i]);
		}
		printf("\n");
	}

	block blockBuffer;
	block prp_block[3];
	AES_KEY prp_block_key[3];
	uint32_t prefix = 0, prefix_shifted, curBlock, pix;
	uint32_t block_mask = (1 << blockLenBit) - 1;
	block_mask <<= (blockLenBit * (blockCount - 1));

	for (int i = 0; i < blockCount; i++) 
	{
		// compute blk_len value
		curBlock = msg & block_mask;
		curBlock >>= blockLenBit * (blockCount - i - 1);

		block_mask >>= blockLenBit;

		//init block`s prp_Key

		setBlock(blockBuffer, i, prefix);

		aes(prp_block[0], blockBuffer, prpKey[0]);
		aes(prp_block[1], blockBuffer, prpKey[1]);
		aes(prp_block[2], blockBuffer, prpKey[2]);

		setKey(prp_block_key[0], (byte*)&prp_block[0], AES_BLOCK_SIZE);
		setKey(prp_block_key[1], (byte*)&prp_block[1], AES_BLOCK_SIZE);
		setKey(prp_block_key[2], (byte*)&prp_block[1], AES_BLOCK_SIZE);

		pix = 0;
		prp((byte*)&pix, (byte*)&curBlock, blockLenBit, prp_block_key);

		prefix_shifted = prefix << blockLenBit;
		setBlock(blockBuffer, (i<<8)|cmp, prefix_shifted | pix);
		aes(leftBase + leftBlockLen * block_loc[i], blockBuffer, trapdoorKey);

		if (DEBUG_INFO)
		{
			printf("Left %d: %.16llx\n", i, prefix_shifted);
		}

		prefix <<= blockLenBit;
		prefix |= curBlock;

	}
	return leftSize;
}

int ORE::genRight(std::string& right, const uint32_t msg, const extraData* data)
{
	const uint32_t blockCount = sizeof(msg) * 8 / blockLenBit;
	const uint32_t blockRows = 1 << blockLenBit;
	const uint32_t cmpLen = cmpLenBit / 8;
	const uint32_t rightBlockLen = cmpLen *blockRows;
	const uint32_t rightSize = AES_BLOCK_SIZE + blockCount * rightBlockLen;

	right.assign(rightSize, 0);
	byte* rightBase = (byte*)right.data();
	
	// gen nonce
	prg(rightBase);
	byte* nonce = rightBase;
	AES_KEY nonceKey;
	setKey(nonceKey, nonce, AES_BLOCK_SIZE);
	rightBase += AES_BLOCK_SIZE;

	// permute block order
	int block_loc[maxBlockCount] = { 0 };
	permuteBlockOrder(block_loc, blockCount);

	if (DEBUG_INFO)
	{
		printf("blockCount:%d\n", blockCount);
		printf("blkLoc enc: ");
		for (int i = 0; i < blockCount; i++)
		{
			printf("%d ", block_loc[i]);
		}
		printf("\n");
	}

	uint8_t cmp;
	block blockBuffer;
	block prp_block[3];
	AES_KEY prp_block_key[3];
	uint32_t prefix = 0, prefix_shifted, curBlock, pi_inv;
	uint32_t block_mask = (1 << blockLenBit) - 1;
	block_mask <<= (blockLenBit * (blockCount - 1));

	for (int i = 0; i < blockCount; i++)
	{
		// compute blk_len value
		curBlock = msg & block_mask;
		curBlock >>= blockLenBit * (blockCount - i - 1);

		block_mask >>= blockLenBit;

		//init block`s prp_Key
		setBlock(blockBuffer, i, prefix);
		aes(prp_block[0], blockBuffer, prpKey[0]);
		aes(prp_block[1], blockBuffer, prpKey[1]);
		aes(prp_block[2], blockBuffer, prpKey[2]);
		setKey(prp_block_key[0], (byte*)&prp_block[0], AES_BLOCK_SIZE);
		setKey(prp_block_key[1], (byte*)&prp_block[1], AES_BLOCK_SIZE);
		setKey(prp_block_key[2], (byte*)&prp_block[1], AES_BLOCK_SIZE);

		prefix_shifted = prefix << blockLenBit;
		if (DEBUG_INFO)
		{
			printf("Right1 %d: %.16llx\n", i, prefix_shifted);
			printf("enc at block %d-->%d, offset_right %d .\n", i, block_loc[i], rightBlockLen * block_loc[i]);
		}

		for (int j = 0; j < blockRows; j++)
		{
			prp((byte*)&pi_inv, (byte*)&j, blockLenBit, prp_block_key, true);

			cmp = (pi_inv == curBlock) ? ORE_EQUAL : ((pi_inv < curBlock) ? ORE_SMALL : ORE_LARGE);
			setBlock(blockBuffer, i << 8 | cmp, prefix_shifted | j);

			aes(blockBuffer, blockBuffer, trapdoorKey);
			aes(blockBuffer, blockBuffer, nonceKey);

			memcpy(rightBase + rightBlockLen * block_loc[i] + (cmpLen)*j, blockBuffer, cmpLen);
		}
	}
	return rightSize;
}

bool ORE::cmp(const std::string & left, const std::string & right)
{
	const uint32_t blockCount = sizeof(uint32_t) * 8 / blockLenBit;
	const uint32_t blockRows = 1 << blockLenBit;
	const uint32_t cmpLen = cmpLenBit / 8;
	const uint32_t leftBlockLen = AES_BLOCK_SIZE;
	const uint32_t leftSize = blockCount * leftBlockLen;
	const uint32_t rightBlockLen = cmpLen * blockRows;
	const uint32_t rightSize = AES_BLOCK_SIZE + blockCount * rightBlockLen;

	byte* leftBase = (byte*)left.data();
	byte* rightBase = (byte*)right.data();

	byte* nonce = rightBase;
	AES_KEY nonceKey;
	setKey(nonceKey, nonce, AES_BLOCK_SIZE);
	rightBase += AES_BLOCK_SIZE;

	// the cmpLen is 64 bit
	uint64_t cmpValue, savValue;
	block blockBuffer;

	for (int i = 0; i < blockCount; i++)
	{
		aes(blockBuffer, leftBase, nonceKey);
		memcpy(&cmpValue, blockBuffer, cmpLen);

		for (int j = 0; j < blockRows; j++)
		{
			memcpy(&savValue, rightBase + j * cmpLen, cmpLen);
			if (cmpValue == savValue)
			{
				if (DEBUG_INFO)
				{
					printf("at block %d:%d, match.\n", i, j);
				}
				return true;
			}
		}

		leftBase += leftBlockLen;
		rightBase += rightBlockLen;
	}
	return false;
}

int ORE::getLeftSize()
{
	const uint32_t blockCount = sizeof(uint32_t) * 8 / blockLenBit;
	const uint32_t leftBlockLen = AES_BLOCK_SIZE;
	const uint32_t leftSize = blockCount * leftBlockLen;
	return leftSize;
}

int ORE::getRightSize()
{
	const uint32_t blockCount = sizeof(uint32_t) * 8 / blockLenBit;
	const uint32_t blockRows = 1 << blockLenBit;
	const uint32_t cmpLen = cmpLenBit / 8;
	const uint32_t rightBlockLen = cmpLen * blockRows;
	const uint32_t rightSize = AES_BLOCK_SIZE + blockCount * rightBlockLen;

	return rightSize;
}

void ORE::setupKey(const std::string & strKey)
{
	memset(masterKey, 0, sizeof(masterKey));
	memcpy(masterKey, strKey.c_str(), sizeof(masterKey)>strKey.size()? strKey.size(): sizeof(masterKey));

	setKey(trapdoorKey, masterKey, AES_BLOCK_SIZE);
	setKey(nonceKey, (byte*)&trapdoorKey, AES_BLOCK_SIZE);
	setKey(prgKey, (byte*)&nonceKey, AES_BLOCK_SIZE);
	setKey(prpKey[0], (byte*)&prgKey, AES_BLOCK_SIZE);
	setKey(prpKey[1], (byte*)&prpKey[0], AES_BLOCK_SIZE);
	setKey(prpKey[2], (byte*)&prpKey[1], AES_BLOCK_SIZE);

	memset(prgCounter, 0, sizeof(prgCounter));

}

void ORE::permuteBlockOrder(int * block_loc, int nblocks)
{
	for (int i = 0; i < maxBlockCount; i++)
	{
		prp((byte*)&block_loc[i], (byte*)&i, 4, prpKey);
	}

	if (nblocks < maxBlockCount)
	{
		for (int i = 0, j=0; i < maxBlockCount; i++)
		{
			if (block_loc[i] < nblocks)
			{
				std::swap(block_loc[i], block_loc[j++]);
			}
		}
	}
}

void ORE::setBlock(byte * dst, uint64_t left, uint64_t right)
{
	memcpy(dst, &left, sizeof(left));
	memcpy(dst + 8, &right, sizeof(right));
}

void ORE::setKey(AES_KEY & key, const byte * src, int size)
{
	byte buffer[AES_BLOCK_SIZE] = { 0 };
	memcpy(buffer, src, size > AES_BLOCK_SIZE ? AES_BLOCK_SIZE : size);
	AES_set_encrypt_key(buffer, 128, &key);
}

void ORE::aes(byte * dst, const byte * src, const AES_KEY& key)
{
	AES_ecb_encrypt(src, dst, &key, AES_ENCRYPT);
}


void ORE::prp(byte * dst, const byte * src, int len, const AES_KEY* keyThree, bool inv)
{
	// AES_KEY keys[3];
	const AES_KEY* keys = keyThree;

	uint32_t val_l, val_r, val_l_new, val_r_new, val = 0;
	uint32_t mask_r = ((((uint32_t)1) << (len / 2)) - 1);
	uint32_t mask_l = mask_r << (len / 2);

	memcpy(&val, src, (len + 7) / 8);

	val_r = val & mask_r;
	val_l = (val & mask_l) >> (len / 2);

	if (inv)
		std::swap(val_l, val_r);

	byte prf_in[AES_BLOCK_SIZE];
	byte prf_out[AES_BLOCK_SIZE];
	uint32_t r;
	const AES_KEY* round_key;
	for (int i = 0; i < 3; i++) 
	{
		round_key = inv ? &keys[2 - i] : &keys[i];

		val_l_new = val_r;

		setBlock(prf_in, 0, val_r);
		aes(prf_out, prf_in, *round_key);

		r = *(uint32_t*)&prf_out;
		r %= ((uint32_t)1) << (len / 2);
		val_r_new = val_l ^ r;

		val_l = val_l_new;
		val_r = val_r_new;
	}

	if (inv)
		std::swap(val_l, val_r);

	uint32_t res = (val_l << (len / 2)) | val_r;
	memcpy(dst, &res, (len + 7) / 8);
}

void ORE::prf(byte * dst, const byte * src, const void * key)
{
	aes(dst, src, *(const AES_KEY*)key);
}

void ORE::prg(byte * dst)
{
	aes(dst, (byte*)&++prgCounter[0], prgKey);
}
