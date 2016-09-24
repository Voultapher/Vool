/*
* Vool - Flat key value container, build on top of std::vector
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#ifndef VOOL_VECMAP_H_INCLUDED
#define VOOL_VECMAP_H_INCLUDED

#include <vector>
#include <algorithm>
#include <iterator>

namespace vool
{

namespace vec_map_util
{

//Partial spezialization
template<typename K, typename V, bool need_reference> class Bucket { };

template<typename K, typename V> class Bucket<K, V, true>
{
public:
	using value_t = typename V;

	Bucket(const K&, const V&);

	Bucket(const Bucket&);
	Bucket(Bucket&&);

	Bucket& operator= (const Bucket&);
	Bucket& operator= (Bucket&&);

	~Bucket() noexcept;

	V& getValue() const { return *_value; }

	const K& getKey() const { return _key; }

	bool operator< (const K& comp) const { return _key < comp; }
	bool operator< (const Bucket& comp) const { return _key < comp._key; }

private:
	K _key;
	V* _value; // more consitent performance for sort and find
};

template<typename K, typename V> class Bucket<K, V, false>
{
public:
	using value_t = typename V;

	Bucket(const K&, const value_t&);

	Bucket(const Bucket&) = default;
	Bucket(Bucket&&) = default;

	Bucket& operator= (const Bucket&) = default;
	Bucket& operator= (Bucket&&) = default;

	~Bucket() noexcept { }

	V& getValue() { return _value; }

	const V& getValue() const { return _value; } // tmp cref

	const K& getKey() const { return _key; }

	bool operator< (const K& comp) const { return _key < comp; }
	bool operator< (const Bucket& comp) const { return _key < comp._key; }

private:
	K _key;
	V _value;
};

}

template<typename K, typename V> class vec_map
{
public:
	//static const bool need_reference = (sizeof(V) >(4 * sizeof(size_t)));
	using bucket_t = typename vec_map_util::Bucket<K, V, (sizeof(V) >(4 * sizeof(size_t)))>;
	using bucket_it_t = typename std::vector<bucket_t>::iterator;

	explicit vec_map();

	vec_map(std::initializer_list<bucket_t>);

	vec_map(const vec_map&) = default;
	vec_map(vec_map&&) = default;

	vec_map& operator= (const vec_map&) = default;
	vec_map& operator= (vec_map&&) = default;

	~vec_map() noexcept { }

	// insert
	void insert(const K& key, const V&);

	void insert(const bucket_t&);

	void insert(
		const typename std::vector<std::pair<K, V>>::iterator,
		const typename std::vector<std::pair<K, V>>::iterator
	);

	void insert(
		const bucket_it_t,
		const bucket_it_t
	);

	void sort();

	void reserve(const size_t);

	void shrink_to_fit();

	void clear();

	// value access
	V& operator[] (const K&);

	V& at(const K&);

	// erase elements
	void erase(const K&);

	void erase(
		const typename std::vector<K>::iterator first,
		const typename std::vector<K>::iterator last
	);

	// bucket range erase: container stays sorted, fastest erase
	void erase(
		const bucket_it_t first,
		const bucket_it_t last
	);

	// iterators
	auto begin() { return _buckets.begin(); }
	auto end() { return _buckets.end(); }

	const auto cbegin() const { return _buckets.cbegin(); }
	const auto cend() const { return _buckets.cend(); }

	auto rbegin() { return _buckets.rbegin(); }
	auto rend() { return _buckets.rend(); }

	const auto crbegin() const { return _buckets.crbegin(); }
	const auto crend() const { return _buckets.crend(); }

	// modifiers
	auto& get_internal_vec() { return _buckets; }

	const auto& get_internal_vec_const() const { return _buckets; }

	// capacity
	const size_t size() const { return _buckets.size(); }

	const size_t capacity() const { return _buckets.capacity(); }

	bool is_sorted() const { return _is_sorted; }

private:
	bool _is_sorted;

	std::vector<bucket_t> _buckets;
};


// ----- IMPLEMENTATION -----


// --- Bucket ---

namespace vec_map_util
{

// -true specialization-

template<typename K, typename V> Bucket<K, V, true>::Bucket(const K& k, const V& v) :
	_key(k),
	_value(new V(v))
{ }

// copy constructor
template<typename K, typename V> Bucket<K, V, true>::Bucket(const Bucket<K, V, true>& other) :
	_key(other._key),
	_value(new V(*other._value))
{ }

// move constructor
template<typename K, typename V> Bucket<K, V, true>::Bucket(Bucket<K, V, true>&& other) :
	_key(std::move(other._key)),
	_value(other._value) // no new alloc needed
{
	other._value = nullptr;
}

// copy operator
template<typename K, typename V> Bucket<K, V, true>& Bucket<K, V, true>::operator= (
	const Bucket<K, V, true>& other
)
{
	if (this != std::addressof(other))
	{
		_key = other._key;
		_value = new V(other.getValue());
	}
	return *this;
}

// move operator
template<typename K, typename V> Bucket<K, V, true>& Bucket<K, V, true>::operator= (
	Bucket<K, V, true>&& other
)
{
	if (this != std::addressof(other))
	{
		_key = std::move(other._key);
		_value = other._value; // no new alloc needed
		other._value = nullptr;
	}
	return *this;
}

template<typename K, typename V> Bucket<K, V, true>::~Bucket() noexcept
{
	delete _value;
}


// -false specialization-

template<typename K, typename V> Bucket<K, V, false>::Bucket(
	const K& key,
	const value_t& value
)
	: _key(key), _value(value)
{ }

}


// --- vec_map ---

template<typename K, typename V> vec_map<K, V>::vec_map() :
	_is_sorted(true)
{ }

// initializer_list constructor
template<typename K, typename V> vec_map<K, V>::vec_map(
	std::initializer_list<bucket_t> init
) :
	_is_sorted(false),
	_buckets(init.begin(), init.end())
{ }


// insert
template<typename K, typename V> void vec_map<K, V>::insert(const K& key, const V& value)
{
	// single element insert
	_buckets.emplace_back(key, value);
	_is_sorted = false;
}

template<typename K, typename V> void vec_map<K, V>::insert(
	const bucket_t& bucket
)
{
	// bucket insert
	_buckets.emplace_back(bucket.getKey(), bucket.getValue());
	_is_sorted = false;
}

template<typename K, typename V> void vec_map<K, V>::insert(
	const typename std::vector<std::pair<K, V>>::iterator first,
	const typename std::vector<std::pair<K, V>>::iterator last
)
{
	for (auto keyIt = first; keyIt != last; ++keyIt)
		insert(keyIt->first, keyIt->second);
}

template<typename K, typename V> void vec_map<K, V>::insert(
	const bucket_it_t first,
	const bucket_it_t last
)
{
	// bucket range insert
	size_t new_size = size() + std::distance(first, last);
	reserve(new_size);
	std::copy(first, last, std::back_inserter(_buckets));
	_is_sorted = false;
}

template<typename K, typename V> void vec_map<K, V>::sort()
{
	std::sort(_buckets.begin(), _buckets.end());
	_is_sorted = true;
}

template<typename K, typename V> void vec_map<K, V>::reserve(const size_t max)
{
	_buckets.reserve(max);
}

template<typename K, typename V> void vec_map<K, V>::shrink_to_fit()
{
	_buckets.shrink_to_fit();
}

template<typename K, typename V> void vec_map<K, V>::clear()
{
	_buckets.clear();
}

// value access
template<typename K, typename V> V& vec_map<K, V>::operator[] (const K& key)
{
	// may crash or return wrong value if used with invalid key
	if (!_is_sorted) sort();
	return std::lower_bound(_buckets.begin(), _buckets.end(), key)->getValue();
}

template<typename K, typename V> V& vec_map<K, V>::at(const K& key)
{
	// should throw properly if used with invalid key
	if (!_is_sorted) sort();
	auto it = std::lower_bound(_buckets.begin(), _buckets.end(), key);
	if (it != _buckets.end() && it->getKey() == key)
		return it->getValue();
	else
		throw std::out_of_range("vec_map key was not valid!");
}

// erase elements
template<typename K, typename V> void vec_map<K, V>::erase(const K& key)
{
	// key erase: container stays sorted
	if (!_is_sorted) sort();
	auto first = std::lower_bound(_buckets.begin(), _buckets.end(), key);
	if (first != _buckets.end())
	{
		_buckets.erase(first);
	}
}

template<typename K, typename V> void vec_map<K, V>::erase(
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

// bucket range erase: container stays sorted, fastest erase
template<typename K, typename V> void vec_map<K, V>::erase(
	const bucket_it_t first,
	const bucket_it_t last
)
{
	_buckets.erase(first, last);
}

}

#endif // VOOL_VECMAP_H_INCLUDED