/*
* Vool - Unit tests for Vecmap, should basically call every member function and test for expected behaviour
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <cstdio>
#include <vector>

#include <Vecmap.h>

namespace vool
{

struct BigData
{
	int sampleArray[40];
};

void test_Vecmap()
{
	// configuration
	using K = size_t;
	using V = BigData;
	K containerSize = static_cast<K>(1e4);

	// Test
	BigData value; // custom value type
	value.sampleArray[0] = 33;

	vool::vec_map<K, V> vecMap(static_cast<size_t>(1e6)); // construction and reserve
	for (size_t key = 0; key < containerSize; ++key)
		vecMap.insert(key, value); // key value insertion

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

	vool::vec_map<K, V> noReserve; // default construction
	for (size_t key = containerSize; key < containerSize * 1.5 + 1; ++key)
		noReserve.insert(key, value); // key value insertion

	// begin() and end() test + bucket range insert
	vecMap.insert(noReserve.begin(), noReserve.end());

	if (vecMap.size() != containerSize * 1.5 + 1)
		throw std::exception("bucket range insert error");

	std::vector<std::pair<K, V>> keyAndValueVec;
	keyAndValueVec.reserve(containerSize);
	for (size_t key = 0; key < containerSize; ++key)
		keyAndValueVec.push_back(std::make_pair(key, value));

	vool::vec_map<K, V> range;
	range.insert(keyAndValueVec.begin(), keyAndValueVec.end()); // key value insert
	if (range.size() != keyAndValueVec.size())
		throw std::exception("key value range insert error");

	std::vector<K> keyVec;
	keyVec.reserve(containerSize);
	for (const auto& key : keyAndValueVec)
		keyVec.push_back(key.first);

	range.erase(keyVec.begin(), keyVec.end()); // key range erase
	if (range.size() != 0)
		throw std::exception("key value range erase error");

	// forced relocation
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

	auto frontKey = vecMap.begin()->getKey();
	vecMap.erase(frontKey); // key erase
	vecMap.insert(frontKey, value);
	if (vecMap.is_sorted())
		throw std::exception("after a key value insertion, the container should be unsorted");

	auto vecMapSize = vecMap.size();
	vecMap.reserve(vecMapSize * 2);
	vecMap.shrink_to_fit(); // should reduce capacity down to size
	if (vecMap.capacity() != vecMapSize)
		throw std::exception("shrink_to_fit error");

	vecMapCopy = vecMap;
	vecMapCopy.clear(); // calls internal vector clear()
	if (vecMapCopy.size() != 0)
		throw std::exception("clear error");

}

}