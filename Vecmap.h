/*
* Vool - Flat key value container, build on top of std::vector
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <vector>
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace vool
{

namespace vec_map_util
{

//Partial spezialization
template<typename K, typename V, bool need_reference> struct Bucket { };

template<typename K, typename V> struct Bucket<K, V, true>
{
private:
	K key;
	V* value; // more consitent performance for sort and find
public:
	using value_t = decltype(value);

	Bucket(const K& k, const V& v) :
		key(k),
		value(new V(v))
	{ }

	// copy constructor
	Bucket(const Bucket<K, V, true>& other) :
		key(other.key),
		value(new V(*other.value))
	{ }

	// move constructor
	Bucket(Bucket<K, V, true>&& other) :
		key(std::move(other.key)),
		value(other.value) // no new alloc needed
	{
		other.value = nullptr;
	}

	// copy operator
	Bucket<K, V, true>& operator= (const Bucket<K, V, true>& other)
	{
		if (this != &other)
		{
			key = other.key;
			value = new V(other.getValue());
		}
		return *this;
	}

	// move operator
	Bucket<K, V, true>& operator= (Bucket<K, V, true>&& other)
	{
		if (this != &other)
		{
			key = std::move(other.key);
			value = other.value; // no new alloc needed
			other.value = nullptr;
		}
		return *this;
	}

	~Bucket() noexcept { delete value; }

	V& getValue() const { return *value; }

	const K& getKey() const { return key; }

	bool operator== (const K& comp) const { return key == comp; }
	bool operator== (const Bucket<K, V, true>& comp) const { return key == comp.key; }

	bool operator!= (const K& comp) const { return key != comp; }
	bool operator!= (const Bucket<K, V, true>& comp) const { return key != comp.key; }

	bool operator< (const K& comp) const { return key < comp; }
	bool operator< (const Bucket<K, V, true>& comp) const { return key < comp.key; }

	bool operator<= (const K& comp) const { return key <= comp; }
	bool operator<= (const Bucket<K, V, true>& comp) const { return key <= comp.key; }

	bool operator> (const K& comp) const { return key > comp; }
	bool operator> (const Bucket<K, V, true>& comp) const { return key > comp.key; }

	bool operator>= (const K& comp) const { return key >= comp; }
	bool operator>= (const Bucket<K, V, true>& comp) const { return key >= comp.key; }
};

template<typename K, typename V> struct Bucket<K, V, false>
{
public:
	K key;
	V value; // more consitent performance for sort and find
public:
	using value_t = decltype(value);

	Bucket(const K& k, const value_t& v) : key(k), value(v) { }

	~Bucket() noexcept { }

	V& getValue() { return value; }

	const V& getValue() const { return std::cref(value); }

	const K& getKey() const { return key; }

	bool operator== (const K& comp) const { return key == comp; }
	bool operator== (const Bucket<K, V, false>& comp) const { return key == comp.key; }

	bool operator!= (const K& comp) const { return key != comp; }
	bool operator!= (const Bucket<K, V, false>& comp) const { return key != comp.key; }

	bool operator< (const K& comp) const { return key < comp; }
	bool operator< (const Bucket<K, V, false>& comp) const { return key < comp.key; }

	bool operator<= (const K& comp) const { return key <= comp; }
	bool operator<= (const Bucket<K, V, false>& comp) const { return key <= comp.key; }

	bool operator> (const K& comp) const { return key > comp; }
	bool operator> (const Bucket<K, V, false>& comp) const { return key > comp.key; }

	bool operator>= (const K& comp) const { return key >= comp; }
	bool operator>= (const Bucket<K, V, false>& comp) const { return key >= comp.key; }
};

}

template<typename K, typename V> struct vec_map
{
private:
	static const bool need_reference = (sizeof(V) > (4 * sizeof(size_t)));

	using bucket_t = vec_map_util::Bucket<K, V, need_reference>;
	std::vector<bucket_t> _buckets;

	bool _is_sorted;

	void copy_internal(const vec_map<K, V>& other)
	{
		reserve(other.capacity());
		if (other.is_referenced())
			for (const auto& bucket : other) // deep copy is needed
				insert(bucket.getKey(), bucket.getValue());
		else
			_buckets = other.get_internal_vec_const();
	}

public:
	// construct
	explicit vec_map() : _is_sorted(true) { }

	explicit vec_map(size_t max) : _is_sorted(false) { reserve(max); }

	vec_map(const vec_map<K, V>& other) : _is_sorted(other.is_sorted())
	{
		copy_internal(std::forward<decltype(other)>(other));
	}

	explicit vec_map(vec_map<K, V>&& other) :
		_is_sorted(other.is_sorted()),
		_buckets(std::move(other._buckets))
	{ }

	explicit vec_map(std::initializer_list<bucket_t> init) : _is_sorted(false)
	{
		_buckets.assign(init.begin(), init.end());
	}

	~vec_map() { }

	vec_map<K, V>& operator= (const vec_map<K, V>& other) noexcept
	{
		// copy assignment
		if (this != &other)
		{
			_is_sorted = other.is_sorted();
			copy_internal(std::forward<decltype(other)>(other));
		}
		return *this;
	}

	vec_map<K, V>& operator= (vec_map<K, V>&& other) //noexcept
	{
		// move assignment
		if (this != &other)
		{
			_is_sorted = other.is_sorted();
			_buckets.reserve(other.capacity());
			_buckets = std::move(other.get_internal_vec());
		}
		return *this;
	}

	// insert
	void insert(const K& key, const V& value)
	{ // single element insert
		_buckets.emplace_back(key, value);
		_is_sorted = false;
	}

	void insert(const decltype(_buckets.back())& bucket)
	{ // bucket insert
		_buckets.emplace_back(bucket.getKey(), bucket.getValue());
		_is_sorted = false;
	}

	void insert(
		const typename std::vector<std::pair<K, V>>::iterator first,
		const typename std::vector<std::pair<K, V>>::iterator last
	)
	{
		for (auto keyIt = first; keyIt != last; ++keyIt)
			insert(keyIt->first, keyIt->second);
	}

	void insert(
		const decltype(_buckets.begin()) first,
		const decltype(_buckets.end()) last
	)
	{ // bucket range insert
		size_t new_size = size() + std::distance(first, last);
		reserve(new_size);
		if (need_reference) // deep copy, as there is no std::back_emplace
			for (auto it = first; it != last; ++it)
				insert(*it);
		else
			std::copy(first, last, std::back_inserter(_buckets));
		_is_sorted = false;
	}

	void sort()
	{
		std::sort(_buckets.begin(), _buckets.end());
		_is_sorted = true;
	}

	void reserve(const size_t max)
	{
		_buckets.reserve(max);
	}

	void shrink_to_fit()
	{
		_buckets.shrink_to_fit();
	}

	void clear()
	{
		_buckets.clear();
	}

	// value access
	V& operator[] (const K& key) // very fast but no checking for wrong key
	{
		// may crash or return wrong value if used with invalid key
		if (!_is_sorted) sort();
		return std::lower_bound(_buckets.begin(), _buckets.end(), key)->getValue();
	}

	V& at(const K& key) // should throw properly if used with invalid key
	{
		if (!_is_sorted) sort();
		auto it = std::lower_bound(_buckets.begin(), _buckets.end(), key);
		if (it != _buckets.end() && it->getKey() == key)
			return it->getValue();
		else
			throw std::out_of_range("vec_map key was not valid!");
	}

	// erase elements
	void erase(const K& key)
	{ // key erase: container stays sorted
		if (!_is_sorted) sort();
		auto first = std::lower_bound(_buckets.begin(), _buckets.end(), key);
		if (first != _buckets.end())
		{
			_buckets.erase(first);
		}
	}

	void erase(
		const typename std::vector<K>::iterator first,
		const typename std::vector<K>::iterator last
	)
	{
		// key range erase: container stays sorted.
		// Should throw if either heighest or lowest key are not valid

		if (first != last)
		{
			std::vector<K> local;
			local.reserve(std::distance(first, last));
			local.assign(first, last);

			std::sort(local.begin(), local.end());
			auto start = std::lower_bound(_buckets.begin(), _buckets.end(), local.front());
			auto end = std::lower_bound(_buckets.begin(), _buckets.end(), local.back());
			if (start != _buckets.end() && end != _buckets.end())
				erase(start, ++end); // end has to point behind last erase key
			else
				throw std::out_of_range("Either the highest or lowest key was not valid!");
		}
	}

	void erase(
		const decltype(_buckets.begin()) first,
		const decltype(_buckets.end()) last
	)
	{ // bucket range erase: container stays sorted, fastest erase
		if (first != _buckets.end())
		{
			_buckets.erase(first, last);
		}
	}

	// get members
	auto& front() const { return _buckets.front(); }

	auto& back() const { return _buckets.back(); }

	auto begin() { return _buckets.begin(); }

	auto end() { return _buckets.end(); }

	const auto begin() const { return _buckets.begin(); }

	const auto end() const { return _buckets.end(); }

	auto& get_internal_vec() { return _buckets; }

	const auto& get_internal_vec_const() const { return _buckets; }

	bool is_sorted() const { return _is_sorted; }

	const size_t size() const { return _buckets.size(); }

	const size_t capacity() const { return _buckets.capacity(); }

	static constexpr bool is_referenced() { return need_reference; }
};

}