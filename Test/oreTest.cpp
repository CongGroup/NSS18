#include "../fastore/ore.h"
#include <time.h>
#include <iostream>
#include <random>
#include <cstdio>
#include <ctime>
#include <string>

using namespace std;

#define TEST_MAX_ROUND 10000

//#define ORG_ORE 1

int main(int argc, char** argv)
{
	const int boundary = 128;

	string* left  = new string[boundary]; 
	string* left2 = new string[boundary];
	string* right = new string[boundary];

	default_random_engine e(time(0));
	uniform_int_distribution<int> dis(0, 0x7fffffff);
	uniform_int_distribution<int> dis_min(0, 0x7fffffff);
	//uniform_int_distribution<int> dis_max(0x40000000, 0x7fffffff);

	unsigned int minValues[boundary];
	unsigned int maxValues[boundary];
	unsigned int queryValues[boundary];
	unsigned int resValues[boundary];

	for (int i = 0; i < boundary; i++)
	{
		minValues[i] = dis_min(e);
		//maxValues[i] = dis_max(e);
		queryValues[i] = dis(e);
		//minValues[i] = 0x00000000;
		//maxValues[i] = 0xffffffff;
		//queryValues[i] = 0x77777777;
		resValues[i] = (queryValues[i]>minValues[i]) ? 1 : -1;
	}


	

	int blk_len = 2;
	if (argc > 1)
	{
		sscanf(argv[1], "%d", &blk_len);
		cout << "blk_len reset to " << blk_len << endl;
	}
	
	ORE engine(blk_len);

	//the block_len is ore block len, [2|4|8]
	engine.setupKey("Hello world!");

	if (true)
	{
		cout << "Begin enc data." << endl;
		timespec start, end;
		double totalTime = 0;
		int round = TEST_MAX_ROUND / 100;
		clock_gettime(CLOCK_REALTIME, &start);
		while (round--)
		{
			for (int i = 0; i < boundary; i++)
			{
				engine.genRight(right[i], minValues[i]);
			}
		}
		clock_gettime(CLOCK_REALTIME, &end);
		totalTime = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_nsec - start.tv_nsec) / 1000.0;
		cout << "Avg enc time is " << totalTime / boundary / (TEST_MAX_ROUND / 100) << " us." << endl;

		for (int i = 0; i < boundary; i++)
		{
			engine.genLeft(left[i], queryValues[i], 1);
			engine.genLeft(left2[i], queryValues[i], -1);
		}

		cout << "Begin query." << endl;
		round = TEST_MAX_ROUND;

		clock_gettime(CLOCK_REALTIME, &start);
		while (round--)
		{
			for (int i = 0; i < boundary; i++)
			{
				bool res1 = engine.cmp(left[i], right[i]);
				bool res2 = engine.cmp(left2[i], right[i]);
				bool res = res1^res2;
				if (!(res&&(resValues[i]?res1:res2)))
				{
					printf("error %d-res %d : %.8lx, %.8lx, %.8lx\n", i, res1^res2, minValues[i], maxValues[i], queryValues[i]);
					return -1;
				}

			}
		}
		clock_gettime(CLOCK_REALTIME, &end);

		totalTime = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_nsec - start.tv_nsec) / 1000.0;
		totalTime /= 2;
		
		cout << "Avg cmp time is " << totalTime / boundary / TEST_MAX_ROUND << " us." << endl;
		cout << "Bye." << endl;

	}
	else
	{
		int min   = 17436672;
		int max   = 17436688;

		int query = 17436673;
		int res;

		engine.genRight(right[0], min);
		engine.genLeft(left[0], query, 1);
		engine.genLeft(left2[0], query, -1);
		
		bool res1 = engine.cmp( left[0], right[0]);
		bool res2 = engine.cmp(left2[0], right[0]);
		cout << "cmp Res : " << res1<<"    "<<res2 << endl;
	}
}