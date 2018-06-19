#include "config.h"


#include "../Caravel/RedisHelper.h"
#include "../Caravel/ThriftAdapt.h"
#include "../Caravel/TimeDiff.h"
#include "../fastore/ore.h"
#include "../thrift/dbService.h"

#include "Proxy.h"

using namespace caravel;

int main(int argc, char ** argv)
{

	if (argc >= 5 && argc <= 8)
	{
		int type = 1;
		int oper = 0;
		int count = 0;
		int offset = 0;
		int scale = 1;
		int blockSize = 0;
		const char* dicPath;
		//const char* outPath;


		sscanf(argv[1], "%d", &type);
		sscanf(argv[2], "%d", &oper);
		sscanf(argv[3], "%d", &count);
		sscanf(argv[4], "%d", &blockSize);

		if (type != 1 && type != 2)
		{
			cout << "Error type." << endl;
			return -1;
		}
		else
		{
			if (type == 1)
			{
				dicPath = "equ.dic";
			}
			else
			{
				dicPath = "rng.dic";
			}
		}

		if (argc > 5)
		{
			sscanf(argv[5], "%d", &offset);
		}

		if (argc > 6)
		{
			sscanf(argv[6], "%d", &scale);
		}

		if (argc > 7)
		{
			dicPath = argv[7];
		}

		Proxy proxy(type);
		proxy.loadDic(dicPath, type);
		proxy.extendToSize(count + offset);

		ThriftAdapt<DSSE::dbServiceClient> dbclient;
		dbclient.Init(THRIFT_IP, THRIFT_PORT);
		dbclient.Open();

		string key;
		string value;
		string left;
		if (type == 2)
		{
			proxy.setOreBlockSize(blockSize);
		}
			
		caravel::TimeDiff::DiffTimeInMicroSecond();

		switch (oper)
		{
		case 1:
		{
			int word;
			string key;
			int ret;

			int resCount = 0;
			for (int i = 0; i < count; i++)
			{
				word = offset + i;
				proxy.getIndexTropdoor(word, proxy[word].count-1, &key, 0);
				if (type == 2)
				{
					proxy.getOreLeft(left);
				}
				ret = dbclient.GetClient()->queryWord(key, proxy[word].key, left);

				resCount += ret;
			}

			printf("query res total %d. \n", resCount);
			break;
		}
		case 2:
		{
			int word;
			string key;
			string value;

			proxy.extendToSize(count + offset + 1);

			while (scale-- > 0)
			{
				for (int i = 0; i < count; i++)
				{
					word = offset + i;
					proxy.encIndexItem(word, &key, &value);
					dbclient.GetClient()->addWord(key, value);

					proxy[word].count += 1;
				}
			}
			break;
		}
		case 3:
		{
			// We choose the top one to delete, it is the quickest case.
			int word;
			string key;
			string* nextKey = 0;

			proxy.extendToSize(count + offset + 1);

			while (scale-- > 0)
			{
				for (int i = 0; i < count; i++)
				{
					word = offset + i;
					if (proxy[word].count > 0)
					{
						proxy[word].count -= 1;
						proxy.getIndexTropdoor(word, proxy[word].count, &key, nextKey);

						dbclient.GetClient()->deleteWord(key, key, proxy[word].key);
					}
				}
			}
			break;
		}
		case 4:
		{
			// type 4 is app index
			proxy.appDic(count, offset, scale);
		}
		default:
		{
			break;
		}
		}

		int time = caravel::TimeDiff::DiffTimeInMicroSecond();

		cout << "#####Useing " << time << " us." << endl;

		if(oper!=1)
			proxy.saveDic(dicPath);

		dbclient.Close();

		return 0;
	}
	else
	{
		cout << "Usage: proxy type oppo count oreBlockSize offset scale dicFile" << endl;
		cout << endl;
		cout << "<type>                = > build Type : 1.equ 2.rng       " << endl;
		cout << "<oppo>                = > operation : 1.que 2.add 3.del  " << endl;
		cout << "<count>               = > word count                     " << endl;
		cout << "<oreBlockSize>        = > word count                     " << endl;
		
		cout << "[offset]              = > word offset                    " << endl;
		cout << "[scale]               = > repect times, default is one   " << endl;
		cout << "[dicFile]             = > proxy index file path          " << endl;
		cout << endl;
		return -1;
	}


}