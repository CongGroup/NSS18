#include "config.h"


#include "../Caravel/RedisHelper.h"
#include "../Caravel/ThriftAdapt.h"
#include "../Caravel/TimeDiff.h"
#include "../fastore/ore.h"

#include "../Proxy/Proxy.h"




using namespace std;
using namespace caravel;

int main(int argc, char ** argv)
{

	if (argc >= 4 && argc <= 7)
	{
		int type = 1;
		int count = 0;
		int blockSize = 0;
		int offset = 0;
		int scale = 1;
		const char* dicPath;
		//const char* outPath;


		sscanf(argv[1], "%d", &type);
		sscanf(argv[2], "%d", &count);
		sscanf(argv[3], "%d", &blockSize);

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

		if (argc > 4)
		{
			sscanf(argv[4], "%d", &offset);
		}
		if (argc > 5)
		{
			sscanf(argv[5], "%d", &scale);
		}
		if (argc > 6)
		{
			dicPath = argv[6];
		}

		Proxy proxy(type);
		if(type == 2)
			proxy.setOreBlockSize(blockSize);
		proxy.loadDic(dicPath, type);
		proxy.extendToSize(count+offset);

		RedisHelper redis;
		redis.Open();

		caravel::TimeDiff::DiffTimeInMicroSecond();

		int word;
		string key;
		string value;
		while (scale-- > 0)
		{
			for (int i = 0; i < count; i++)
			{
				word = offset + i;
				proxy.appDic(1, word);
				proxy.encIndexItem(word, &key, &value);
				redis.Put(key, value);
			}
		}

		int time = caravel::TimeDiff::DiffTimeInMicroSecond();
		cout << "#####build use " << time << " us." << endl;

		redis.Close();
		proxy.saveDic(dicPath);

		return 0;
	}
	else
	{
		cout << "Usage: builder type count oreBlockSize offset scale dicFile" << endl;
		cout << endl;
		cout << "<type>                = > build Type : 1.equ 2.rng    " << endl;
		cout << "<count>               = > insert word count           " << endl;
		cout << "<oreBlockSize>        = > ore block bit length        " << endl;
		cout << "[offset]              = > insert word offset          " << endl;
		cout << "[scale]               = > repect times                " << endl;
		cout << "[dicFile]             = > proxy index file path       " << endl;
		cout << endl;
		return -1;
	}


}