#pragma once
#include <string>
#include <vector>
#include <openssl/aes.h>

#include "config.h"

#include "../fastore/ore.h"
#include "../thrift/dbService.h"

//#include <map>

using std::string;
using std::vector;

struct WordDic
{
	string word;
	int count;
	string key;
};

class Proxy
{
public:
	Proxy(int type);
	~Proxy();

	// save dic to file
	void saveDic(string filePath);
	// load dic from file
	void loadDic(string filePath, int expectType);
	// just app index count word with offset
	void appDic(int count, int offset, int scale = 1);
	//// add word in index and db
	//void addWord(int count, int offset, int scale = 1);
	//// del word in index and db
	//void delWord(int count, int offset, int scale = 1);
	//// query word
	//void query(int count, int offset = 0);
	// encrypt db item, the key not null
	void encIndexItem(const int word, string* key, string* value);
	// encrypt proxy query, the key not null
	void getIndexTropdoor(const int word, const int counter, string* key, string* nextkey);
	// decrypte db item
	string decNextKey(const string* value, string* nextkey, const string* left);
	// update value
	void updateKey(string& value, const string newKey);
	// extend dic max size
	void extendToSize(int size);
	// set ore block size
	void setOreBlockSize(int size);
	// get ore left query
	void getOreLeft(string& left);
	// draw sketch of index
	void sketch(void);

	WordDic& operator [] (const int n)
	{
		return proxyDic[n];
	}
	
private:
	int type; // 1:means equ  2:means rng
	vector<WordDic> proxyDic;
	AES_KEY encKey;
	AES_KEY decKey;
	ORE* pOREngine;
	int oreBlockSize;
};

