/*
* Vool - Unit tests for vec_map
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#include "AllTests.h"

#include <Vecmap.h>

#include <vector>
#include <exception>

namespace vool
{

namespace tests
{

struct BigData
{
	using array_t = int;
	static const size_t size = (sizeof(size_t) * 6) / sizeof(array_t);
	array_t sampleArray[size];
};


void test_Vecmap()
{
	// configuration
	using K = size_t;
	using V = BigData;
	size_t containerSize = static_cast<K>(1e4);

	static_assert(std::is_same<
			vool::vec_map<K, V>::bucket_t,
			vool::vec_map_util::ref_bucket<K, V>
		>::value, "vec_map bucket selection fail!");


	V value; // custom value type
	value.sampleArray[0] = 33;

	// size construction
	vool::vec_map<K, V> vecMap;
	vecMap.reserve(containerSize);
	for (size_t key = 0; key < containerSize; ++key)
		vecMap.insert(key, value);

	if (vecMap.size() != containerSize)
		throw std::exception("key value insert error");

	if (vecMap[containerSize - 1].sampleArray[0] != value.sampleArray[0])
		throw std::exception("key value insert error");

	// test if returned value is actually what we looked for
	{
		vool::vec_map<K, K> vMap({ { 0, 0 },{ 1, 1 },{ 3, 3 } });

		bool access = false;

		try
		{
			auto res = vMap.at(2);
			static_cast<void>(res);

			access = true;
		}
		catch (std::exception& e) { static_cast<void>(e); }; // this should fail

		if (access)
			throw std::exception("could access value using at() with wrong key");

	}

	// begin() and end() test and bucket range insert test
	{
		V::array_t rangeLastItem = 5;

		{
			vool::vec_map<K, V> noReserve; // default construction
			for (size_t key = containerSize; key < containerSize * 1.5; ++key)
				noReserve.insert(key, value); // key value insertion

			noReserve[0].sampleArray[V::size - 1] = rangeLastItem;

			vecMap.insert(noReserve.begin(), noReserve.end());
		}

		if (vecMap.size() != containerSize * 1.5)
			throw std::exception("bucket range insert size error");

		if (vecMap[containerSize].sampleArray[V::size - 1] != rangeLastItem)
			throw std::exception("bucket range insert copy error");
	}

	{
		std::vector<std::pair<K, V>> keyAndValueVec;
		keyAndValueVec.reserve(containerSize);
		for (size_t key = 0; key < containerSize; ++key)
			keyAndValueVec.push_back(std::make_pair(key, value));

		vool::vec_map<K, V> range;
		std::for_each(keyAndValueVec.begin(), keyAndValueVec.end(),
			[&range](const auto& kvPair)
			{ range.insert(kvPair.first, kvPair.second); }
		);
		if (range.size() != keyAndValueVec.size())
			throw std::exception("key value range insert error");

		std::vector<K> keyVec;
		keyVec.reserve(containerSize);
		for (const auto& key : keyAndValueVec)
			keyVec.push_back(key.first);

		range.erase(keyVec.begin(), keyVec.end()); // key range erase
		if (range.size() != 0)
			throw std::exception("key value range erase error");
	}

	// forced relocation
	{
		auto tmp = vecMap; // copy assignment
		vecMap = std::move(tmp); // move assignment

		auto copyConstructed(vecMap); // copy construction
		auto moveCopy = vecMap;
		auto moveConstructed(std::move(moveCopy)); // move construction

		auto vecMapCopy = vecMap; // copy construct assignment
		auto itStart = vecMapCopy.begin();
		std::advance(itStart, (vecMapCopy.size() / 4));
		auto itEnd = vecMapCopy.begin();
		std::advance(itEnd, (vecMapCopy.size() / 2));

		vecMapCopy.erase(itStart, itEnd); // bucket range erase

		if (vecMapCopy.at(0).sampleArray[0] != value.sampleArray[0])
			throw std::exception("bucket erase and or at() error");
		if (!vecMapCopy.is_sorted())
			throw std::exception("after any kind of read, vec_map should be sorted");
	}

	// capacity tests
	{
		auto frontKey = vecMap.begin()->key();
		vecMap.erase(frontKey); // key erase
		vecMap.insert(frontKey, value);
		if (vecMap.is_sorted())
			throw std::exception("container should be unsorted after key insert");

		auto vecMapSize = vecMap.size();
		vecMap.reserve(vecMapSize * 2);
		vecMap.shrink_to_fit(); // should reduce capacity down to size
		if (vecMap.capacity() != vecMapSize)
			throw std::exception("shrink_to_fit error");

		auto vecMapCopy = vecMap;
		vecMapCopy.clear(); // calls internal vector clear()
		if (vecMapCopy.size() != 0)
			throw std::exception("clear error");
	}

}

}

}