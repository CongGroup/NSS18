namespace cpp DSSE
namespace java com.cityu.dsse

service dbService
{
	i32 queryWord(1:binary trapdoor, 2:binary key, 3:binary left);

	void addWord(1:binary key, 2:binary value);

	void addWords(1:list<binary> keys, 2:list<binary>values);

	void deleteWord(1:binary trapdoor, 2:binary delTrapdoor, 3:binary key);

	void deleteWords(1:list<binary> trapdoors, 2:list<binary>keys);

} 
