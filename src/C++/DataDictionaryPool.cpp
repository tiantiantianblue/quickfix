#include "DataDictionaryPool.h"

namespace FIX
{
	static std::map < std::string, DataDictionary > dictionaries;
	const DataDictionary& FIX::getDataDictionary(const std::string & path)
	{
		auto i = dictionaries.find(path);
		if (i != dictionaries.end())
			return i->second;
		else
		{
			dictionaries[path] = DataDictionary(path);
			return dictionaries[path];
		}
	}
}