#include "Proxy.h"
#include <cstring>
#include <fstream>
#include "../Caravel/AES.h"
#include "../Caravel/Digest.h"


using namespace std;
using namespace caravel;

#define NDEBUG
#include <assert.h>

Proxy::Proxy(int type)
{
	this->type = type;

	pOREngine = 0;
	oreBlockSize = 0;
	AES_set_encrypt_key(MASTER_KEY, 128, &encKey);
	AES_set_decrypt_key(MASTER_KEY, 128, &decKey);
}


Proxy::~Proxy()
{
	if (pOREngine)
	{
		delete pOREngine;
	}
}

// the first value is type
// next is wordDic
void Proxy::saveDic(string filePath)
{
	ofstream fout(filePath, ios::trunc&ios::out&ios::binary);

	fout.write((const char*)&type, sizeof(type));
	
	int dicSize = proxyDic.size();
	fout.write((const char*)&dicSize, sizeof(dicSize));

	int num;
	for (WordDic item : proxyDic)
	{
		num = item.word.size();
		fout.write((const char*)&num, sizeof(num));
		fout.write(item.word.data(), num);

		num = item.count;
		fout.write((const char*)&num, sizeof(num));

		num = item.key.size();
		fout.write((const char*)&num, sizeof(num));
		fout.write(item.key.data(), num);
	}

	fout.close();

	printf("save success:\n");
	sketch();
}

void Proxy::loadDic(string filePath, int expectType)
{
	ifstream fin(filePath, ios::in&ios::binary);
	fin.seekg(0, ios::end);
	if (fin.tellg() == 0)
	{
		type = expectType;

		printf("load empty from file %s.\n", filePath.c_str());
	}
	else
	{
		fin.seekg(0, ios::beg);

		fin.read((char*)&type, sizeof(type));
		int dicSize = 0;
		fin.read((char*)&dicSize, sizeof(dicSize));

		WordDic item;
		int count = 0;
		int num;
		char buffer[512] = { 0 };
		while (!fin.eof()&& dicSize-->0)
		{
			fin.read((char*)&num, sizeof(num));
			fin.read(buffer, num);
			item.word.assign(buffer, num);

			fin.read((char*)&num, sizeof(num));
			item.count = num;

			fin.read((char*)&num, sizeof(num));
			fin.read(buffer, num);
			item.key.assign(buffer, num);
			
			count++;
			proxyDic.push_back(std::move(item));
		}

		printf("load %d word, from file %s.\n", count, filePath.c_str());
	}
	fin.close();

	sketch();
}

void Proxy::appDic(int count, int offset, int scale)
{
	int word;

	extendToSize(count + offset + 1);

	for (int i = 0; i < count; i++)
	{
		word = offset + i;
		proxyDic[word].count += scale;
	}
}

//void Proxy::addWord(int count, int offset, int scale)
//{
//	int word;
//	string key;
//	string value;
//
//	extendToSize(count + offset + 1);
//
//	while (scale-- > 0)
//	{
//		for (int i = 0; i < count; i++)
//		{
//			word = offset + i;
//			proxyDic[word].count += 1;
//			encIndexItem(word, &key, &value);
//			//TODO call thrift to insert key value;
//			dbclient.GetClient()->addWord(key, value);
//		}
//	}
//}
//
//void Proxy::delWord(int count, int offset, int scale)
//{
//	int word;
//	string key;
//	string nextKey;
//
//	extendToSize(count + offset + 1);
//
//	while (scale-- > 0)
//	{
//		for (int i = 0; i < count; i++)
//		{
//			word = offset + i;
//			getIndexTropdoor(word, 0, &key, &nextKey);
//			proxyDic[word].count -= 1;
//			//TODO call thrift to delete key and update nextKey;
//		}
//	}
//}
//
//void Proxy::query(int count, int offset)
//{
//	int word;
//	string key;
//	vector<string> ret;
//
//	int count=0;
//	for (int i = 0; i < count; i++)
//	{
//		word = offset + i;
//		getIndexTropdoor(word, proxyDic[word].count, &key, 0);
//		ret.clear();
//		dbclient.GetClient()->queryWord(ret, key);
//
//		count += ret.size();
//	}
//
//	printf("query res total %d. \n", count);
//}

void Proxy::encIndexItem(const int word, string * key, string * value)
{
	if (!key)
	{
		return;
	}

	int cnt = proxyDic[word].count;
	assert(cnt >= 0);
	uint32_t len, outputLen;
	string msg = string(proxyDic[word].word).append(std::to_string(cnt));

	//printf("enc msg: %s\n", msg.c_str());
	char buffer[64];
	char aesBuffer[64] = { 0 };
	Digest::HMACSha256(MASTER_KEY, strlen(MASTER_KEY), msg.data(), msg.size(), buffer, outputLen);
	assert(outputLen == 32);
	key->assign(buffer, outputLen);

	if (value)
	{
		cnt -=1;
		msg = string(proxyDic[word].word).append(std::to_string(cnt));
		Digest::HMACSha256(MASTER_KEY, strlen(MASTER_KEY), msg.data(), msg.size(), buffer, outputLen);
		len = msg.size();
		memcpy(buffer + 32, &len, sizeof(len));
		memcpy(buffer + 32 + sizeof(len), msg.data(), len);

		len = 32 + sizeof(len) + len;

		len = AES::EcbEncrypt128(buffer, len, aesBuffer, MASTER_KEY);

		if (len > 64)
		{
			printf("long msg size is %d\n", len);
		}

		value->assign(aesBuffer, 64);

		if (type == 2)
		{
			string right;
			pOREngine->genRight(right, word);

			value->append(right);
		}
		
	}
}

void Proxy::getIndexTropdoor(const int word, const int counter, string * key, string * nextkey)
{
	if (!key)
	{
		return;
	}

	int cnt = counter;

	uint32_t outputLen = 32;
	string msg = string(proxyDic[word].word).append(std::to_string(cnt));

	//printf("tropdoor msg: %s\n", msg.c_str());

	char buffer[32];
	Digest::HMACSha256(MASTER_KEY, strlen(MASTER_KEY), msg.data(), msg.size(), buffer, outputLen);
	assert(outputLen == 32);
	key->assign(buffer, 32);

	if (nextkey)
	{
		cnt = cnt + 1;
		msg = string(proxyDic[word].word).append(std::to_string(cnt));
		Digest::HMACSha256(MASTER_KEY, strlen(MASTER_KEY), msg.data(), msg.size(), buffer, outputLen);
		assert(outputLen == 32);
		nextkey->assign(buffer, 32);
	}
}

string Proxy::decNextKey(const string* value, string* nextkey, const string* left)
{
	char aesBuffer[64] = { 0 };

	if (type == 2)
	{
		string right = value->substr(64);
		if (!pOREngine->cmp(*left, right))
		{
			printf("ore failed.\n");
		}
	}

	int len = 64;
	const char* val = value->c_str();
	if (*(uint64_t*)(val + 48) == 0)
	{
		len = 48;
	}

	len = AES::EcbDecrypt128(value->c_str(), len, aesBuffer, MASTER_KEY);

	if (len > 0)
	{
		nextkey->assign(aesBuffer, 32);
	}
	else
	{
		printf("aes dec failed\n;");
	}

	memcpy(&len, aesBuffer + 32, sizeof(len));
	return string(aesBuffer + 32 + sizeof(len), len);

}

void Proxy::updateKey(string& value, const string newKey)
{
	//value.replace(0, 32, AES(newKey.c_str()), 32);
	char aesBuffer[64] = { 0 };
	char aesBuffer2[64] = { 0 };
	
	int len = 64;
	const char* val = value.c_str();
	if (*(uint64_t*)(val + 48) == 0)
	{
		len = 48;
	}

	len = AES::EcbDecrypt128(value.c_str(), len, aesBuffer, MASTER_KEY);

	assert(len > 0);

	memcpy(aesBuffer, newKey.c_str(), 32);

	len = AES::EcbEncrypt128(aesBuffer, len, aesBuffer2, MASTER_KEY);

	assert(len > 0);

	//although the value have ore right, we just replace the first 64 byte.

	value.replace(0, 64, aesBuffer2, 64);
}

void Proxy::extendToSize(int size)
{
	if (proxyDic.size() < size)
	{
		for (int i = proxyDic.size(); i <= size; i++)
		{
			WordDic item;
			char buffer[32] = { 0 };

			switch (type)
			{
			//que
			case 1:
			{
				sprintf(buffer, "e%d", i);
				break;
			}

			//rng
			case 2:
			{
				sprintf(buffer, "%d", i);
				break;
			}
			default:
			{
				printf("error type:%d\n", type);
			}
			}
			item.word = buffer;
			item.count = 0;
			int len = AES::EcbEncrypt128(item.word.data(), item.word.size(), buffer, MASTER_KEY);
			assert(len == 16);
			item.key.assign(buffer, len);

			proxyDic.push_back(std::move(item));
		}
	}
}

// set ore block size
void Proxy::setOreBlockSize(int size) 
{
	if (oreBlockSize != size)
	{
		oreBlockSize = size;
		if (pOREngine)
		{
			delete pOREngine;
		}
		pOREngine = new ORE(oreBlockSize);
		pOREngine->setupKey(MASTER_KEY);
	}

}

void Proxy::getOreLeft(string & left)
{
	pOREngine->genLeft(left, 0, -1);
}

void Proxy::sketch(void)
{
	if (proxyDic.size() < 2)
		return;

	vector<string> vecSketch;
	int beginPos = 0;
	int sketchLevel = proxyDic[0].count;

	for (int i = 1; i < proxyDic.size(); i++)
	{
		if (sketchLevel != proxyDic[i].count)
		{
			string msg;
			msg.append(to_string(beginPos));
			msg.append("\t-->");
			msg.append(to_string(i - 1));
			msg.append("\t has count ");
			msg.append(to_string(sketchLevel));
			msg.append(".\n");
			vecSketch.push_back(msg);

			beginPos = i;
			sketchLevel = proxyDic[i].count;
		}
	}
	
	{
		int i = proxyDic.size();

		string msg;
		msg.append(to_string(beginPos));
		msg.append("\t-->");
		msg.append(to_string(i - 1));
		msg.append("\t has count ");
		msg.append(to_string(sketchLevel));
		msg.append(".\n");
		vecSketch.push_back(msg);
	}

	for (string s : vecSketch)
	{
		printf(s.c_str());
	}
}
