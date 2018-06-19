#ifndef __DIGEST_H__
#define __DIGEST_H__

#include <string>
#include <stdint.h>

namespace caravel 
{

	class Digest
	{
	public:

		static int Sha256(std::string &sMsg, char *szBuf, uint32_t uiLen);

		static int Sha256(const char *szMsg, uint32_t uiMsgLen, char *szBuf, uint32_t uiLen);

		static int Sha256(std::string &sMsg, std::string &sRet);

		static int HMACSha256(const char* key, uint32_t key_size, const char* msg, uint32_t msg_size,const char* output, uint32_t& output_size);
	};

}

#endif
