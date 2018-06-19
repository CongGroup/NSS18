#include "Digest.h"
#include <iostream>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

using namespace std;

namespace caravel {

	int Digest::Sha256(string &sMsg, char *szBuf, uint32_t uiLen)
	{
		return Sha256(sMsg.c_str(), sMsg.length(), szBuf, uiLen);
	}

	int Digest::Sha256(const char *szMsg, uint32_t uiMsgLen, char *szBuf, uint32_t uiLen)
	{
		if (uiLen < SHA256_DIGEST_LENGTH)
		{
			return -1;
		}
		//May not necessary
		memset(szBuf, 0, uiLen);
		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, szMsg, uiMsgLen);
		SHA256_Final((unsigned char*)szBuf, &sha256);
		//cout<<szBuf<<endl;
		return 0;
	}

	int Digest::Sha256(string &sMsg, string &sRet)
	{
		char szBuf[SHA256_DIGEST_LENGTH];
		Sha256(sMsg, szBuf, SHA256_DIGEST_LENGTH);
		sRet.assign(szBuf, SHA256_DIGEST_LENGTH);
		return 0;
	}

	int Digest::HMACSha256(const char * key, uint32_t key_size, const char * msg, uint32_t msg_size, const char * output, uint32_t& output_size)
	{
		::HMAC(EVP_sha256(), key, key_size, (unsigned char*)msg, msg_size, (unsigned char*)output, &output_size);
		return 0;
	}
}