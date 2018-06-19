#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "../thrift/dbService.h"
using namespace  ::DSSE;

#include "../Proxy/Proxy.h"

#include "../Caravel/RedisHelper.h"
using namespace caravel;

int oreBlockSize;

class dbServiceHandler : virtual public dbServiceIf 
{
public:
	dbServiceHandler() 
	{
		// Your initialization goes here
		proxyEqu = new Proxy(1);

		proxyRng = new Proxy(2);
		proxyRng->setOreBlockSize(oreBlockSize);

		redis.Open();
	}

	int32_t queryWord(const std::string& trapdoor, const std::string& key, const std::string& left)
	{
		string value, nextkey;
		int res = 0;

		redis.Get(trapdoor, value);

		if (value.size() == 0)
		{
			//printf("empty query.\n");
			return 0;
		}


		if (left.size() == 0)
		{
			while (value.size() > 0)
			{
				res += 1;
				string data = proxyEqu->decNextKey(&value, &nextkey, 0);
				//printf("%s, ", data.c_str());
				redis.Get(nextkey, value);
			}
		}
		else
		{
			while (value.size() > 0)
			{
				res += 1;
				string data = proxyEqu->decNextKey(&value, &nextkey, &left);
				//printf("%s, ", data.c_str());
				redis.Get(nextkey, value);
			}
		}

		//printf("get res %d.\n", res);
		return res;
	}

	void addWord(const std::string& key, const std::string& value)
	{
		redis.Put(key, value);
	}

	void addWords(const std::vector<std::string> & keys, const std::vector<std::string> & values)
	{
		for (int i = 0; i < keys.size(); i++)
		{
			redis.Put(keys[i], values[i]);
		}
	}

	//1:binary trapdoor, 2:binary delTrapdoor, 3:binary key
	void deleteWord(const std::string& trapdoor, const std::string& delTrapdoor, const std::string& key) 
	{
		// Your implementation goes here
		string value, nextKey, nextValue;

		redis.Get(trapdoor, value);

		if (value.size() == 0)
		{
			//printf("empty del.\n");
			return;
		}

		proxyEqu->decNextKey(&value, &nextKey, 0);

		redis.Put(trapdoor, string());

		if (trapdoor == delTrapdoor)
		{
			return;
		}
		else
		{
			string curKey = trapdoor;
			redis.Get(nextKey, nextValue);
			while (nextValue.size() > 0)
			{
				curKey = nextKey;
				proxyEqu->decNextKey(&nextValue, &nextKey, 0);
				proxyEqu->updateKey(value, nextKey);
				redis.Put(curKey, value);

				if (curKey == delTrapdoor)
				{
					break;
				}
				else
				{
					value = nextValue;
					redis.Get(nextKey, nextValue);
				}
			}
		}
		//printf("del success.\n");
	}

	void deleteWords(const std::vector<std::string> & trapdoors, const std::vector<std::string> & keys) {
		// Your implementation goes here
		//printf("deleteWords\n");
	}

	RedisHelper redis;
	Proxy* proxyEqu;
	Proxy* proxyRng;
};

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: server <ORE_BLOCK_SIZE>\n");
		return 0;
	}

	sscanf(argv[1], "%d", &oreBlockSize);

	int port = 9090;
	boost::shared_ptr<dbServiceHandler> handler(new dbServiceHandler());
	boost::shared_ptr<TProcessor> processor(new dbServiceProcessor(handler));
	boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
	boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
	boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

	TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
	server.serve();
	return 0;



}