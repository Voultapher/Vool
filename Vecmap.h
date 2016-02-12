#pragma once

#include <vector>
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace vool
{

//Partial spezialization
template<typename K, typename V, bool need_reference, typename ENABLE = void> struct Bucket { };

template<typename K, typename V, bool need_reference> struct Bucket
	<K, V, need_reference, typename std::enable_if<need_reference>::type>
{
private:
	K key;
	V* value; // more consitent performance for sort and find
public:
	using value_t = decltype(value);

	explicit Bucket(const K& k, const V& v) : key(k), value(new V(v)) { }

	~Bucket() noexcept { }

	void destroy() { delete value; }

	V& getValue() const { return *value; }

	const K& getKey() const { return key; }

	bool operator== (const K& comp) const { return key == comp; }
	bool operator== (const Bucket<K, V, need_reference>& comp) const { return key == comp.key; }

	bool operator!= (const K& comp) const { return key != comp; }
	bool operator!= (const Bucket<K, V, need_reference>& comp) const { return key != comp.key; }

	bool operator< (const K& comp) const { return key < comp; }
	bool operator< (const Bucket<K, V, need_reference>& comp) const { return key < comp.key; }

	bool operator<= (const K& comp) const { return key <= comp; }
	bool operator<= (const Bucket<K, V, need_reference>& comp) const { return key <= comp.key; }

	bool operator> (const K& comp) const { return key > comp; }
	bool operator> (const Bucket<K, V, need_reference>& comp) const { return key > comp.key; }

	bool operator>= (const K& comp) const { return key >= comp; }
	bool operator>= (const Bucket<K, V, need_reference>& comp) const { return key >= comp.key; }
};

template<typename K, typename V, bool need_reference> struct Bucket
	<K, V, need_reference, typename std::enable_if<!need_reference>::type>
{
public:
	K key;
	V value; // more consitent performance for sort and find
public:
	using value_t = decltype(value);

	explicit Bucket(const K& k, const value_t& v) : key(k), value(v) { }

	~Bucket() noexcept { }

	void destroy() const noexcept { }

	V& getValue() { return value; }

	const V& getValue() const { return std::cref(value); }

	const K& getKey() const { return key; }

	bool operator== (const K& comp) const { return key == comp; }
	bool operator== (const Bucket<K, V, need_reference>& comp) const { return key == comp.key; }

	bool operator!= (const K& comp) const { return key != comp; }
	bool operator!= (const Bucket<K, V, need_reference>& comp) const { return key != comp.key; }

	bool operator< (const K& comp) const { return key < comp; }
	bool operator< (const Bucket<K, V, need_reference>& comp) const { return key < comp.key; }

	bool operator<= (const K& comp) const { return key <= comp; }
	bool operator<= (const Bucket<K, V, need_reference>& comp) const { return key <= comp.key; }

	bool operator> (const K& comp) const { return key > comp; }
	bool operator> (const Bucket<K, V, need_reference>& comp) const { return key > comp.key; }

	bool operator>= (const K& comp) const { return key >= comp; }
	bool operator>= (const Bucket<K, V, need_reference>& comp) const { return key >= comp.key; }
};

template<typename K, typename V> struct vec_map
{
private:
	static const bool need_reference = (sizeof(V) > (2 * sizeof(size_t)));
	bool _is_sorted; // tmp
	std::vector<Bucket<K, V, need_reference>> _buckets;

	void copy_internal(const vec_map<K, V>& other)
	{
		reserve(other.capacity());
		if (other.is_referenced())
			for (const auto& bucket : other) // deep copy is needed
				insert(bucket.getKey(), bucket.getValue());
		else
			_buckets = other.get_internal_vec_const();
	}

	void move_internal(vec_map<K, V>&& other)
	{
		reserve(other.capacity());
		destroy_internal()
			_buckets = std::move(other.get_internal_vec());
	}

	inline void destroy_internal()
	{
		if (need_reference)
			for (auto& bucket : _buckets)
				bucket.destroy();
	}

public:
	// construct
	explicit vec_map() : _is_sorted(true) { }

	explicit vec_map(size_t max) : _is_sorted(false) { reserve(max); }

	vec_map(const vec_map<K, V> const& other) : _is_sorted(other.is_sorted())
	{
		copy_internal(std::forward<decltype(other)>(other));
	}

	explicit vec_map(vec_map<K, V>&& other) : _is_sorted(other.is_sorted())
	{
		move_internal(std::forward<decltype(other)>(other));
	}

	~vec_map()
	{
		destroy_internal();
	}

	vec_map<K, V>& operator= (const vec_map<K, V> const& other) noexcept
	{ // copy assignment
		if (this != &other)
		{
			_is_sorted = other.is_sorted();
			copy_internal(std::forward<decltype(other)>(other));
		}
		return *this;
	}

	vec_map<K, V>& operator= (vec_map<K, V>&& other) //noexcept
	{ // move assignment
		if (this != &other)
		{
			_is_sorted = other.is_sorted();
			move_internal(std::forward<decltype(other)>(other));
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

	void insert(const typename std::vector<std::pair<K, V>>::iterator first, const typename std::vector<std::pair<K, V>>::iterator last)
	{
		for (auto keyIt = first; keyIt != last; ++keyIt)
			insert(keyIt->first, keyIt->second);
	}

	void insert(const decltype(_buckets.begin()) first, const decltype(_buckets.end()) last)
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
		if (need_reference)
			destroy_internal();
		_buckets.clear();
	}

	// get value
	V& operator[] (const K key)
	{
		if (!_is_sorted) sort();
		return std::lower_bound(_buckets.begin(), _buckets.end(), key)->getValue();
	}

	V& at(const K& key)
	{
		if (!_is_sorted) sort();
		auto res = std::lower_bound(_buckets.begin(), _buckets.end(), key);
		if (res != _buckets.end())
			return res->getValue()
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
			first->destroy();
			_buckets.erase(first);
		}
	}

	void erase(const typename std::vector<K>::iterator first, const typename std::vector<K>::iterator last)
	{ // key range erase: container stays sorted. Will throw if either heighest or lowest key are not valid
		if (first != last)
		{
			std::vector<K> local;
			local.reserve(std::distance(first, last));
			local.assign(first, last);

			std::sort(local.begin(), local.end());
			auto start = std::lower_bound(_buckets.begin(), _buckets.end(), local.front());
			auto end = std::lower_bound(_buckets.begin(), _buckets.end(), local.back());
			if (start != _buckets.end() && end != _buckets.end())
				erase(start, end);
			else
				throw std::out_of_range("Either the highest or lowest key was not valid!");
		}
	}

	void erase(const decltype(_buckets.begin()) first, const decltype(_buckets.end()) last)
	{ // bucket range erase: container stays sorted, fastest erase
		if (first != _buckets.end())
		{
			if (need_reference)
				for (auto it = first; it != last; ++it)
					it->destroy();
			_buckets.erase(first, last);

			//shrink_to_fit();
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