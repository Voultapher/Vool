// vool is metaprogramming library that adds functunality to c++ programms

#pragma once

#include <tuple>
#include <utility>
#include <exception>
#include <type_traits>
#include <stdint.h>

// MAKE SURE TO INCLUDE THIS FILE LAST AS IT CAN CAUSE PROBLEMS

namespace vool
{

namespace util
{
	// variadic compile time property check
	template < bool CONDITION > struct true_if : std::conditional< CONDITION, std::true_type, std::false_type >::type {};

	template<typename T> struct is_arithmetic : std::is_arithmetic<T> { };
	template<typename A, typename B> struct is_convertible_to : std::is_convertible<A, B> {};

	template<bool...> struct bool_pack;
	template<bool... b> using all_true = std::is_same<bool_pack<true, b...>, bool_pack<b..., true>>;
	template<bool... b> using all_false = std::is_same<bool_pack<false, b...>, bool_pack<b..., false>>;
	template<bool... b> using some_true = true_if<!all_true<b...>::value && !all_false<b...>::value>;

	template<typename... Ts> struct all_are_arithmetic : true_if<all_true<is_arithmetic<Ts>::value...>::value> {};
	template<typename TO, typename... FROM> struct all_are_convertible : true_if<all_true<is_convertible_to<TO, FROM>::value...>::value> {};

	// tuple iteration
	template<typename... Ts> void wrapper(Ts&&... args) { }

	// simple iteratio, only tuple as argument
	template<class F, class... Ts, std::size_t... Is>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>) {
		wrapper((func(std::get<Is>(tuple)), 0)...);
	}

	template<class F, class...Ts>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func) {
		for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
	}

	//simple iteration, with tuple + scalar
	template<typename... Ts, typename S, typename F, std::size_t... Is>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, S& scalar, F func, std::index_sequence<Is...>) {
		wrapper((func(std::get<Is>(tuple), scalar), 0)...);
	}

	template<typename... Ts, typename S, typename F>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, S& scalar, F func) {
		for_each_in_tuple(tuple, scalar, func, std::make_index_sequence<sizeof...(Ts)>());
	}

	//simple iteration, with 3 tuples
	template<typename... Ret, typename... A, typename... B, class F, std::size_t... Is>
	void tuple_operator(std::tuple<Ret...>& ret, const std::tuple<A...>& a, const std::tuple<B...>& b, F func, std::index_sequence<Is...>) {
		wrapper((func(std::get<Is>(ret), std::get<Is>(a), std::get<Is>(b)), 0)...);
	}

	template<typename... Ret, typename... A, typename... B, class F>
	void tuple_operator(std::tuple<Ret...>& ret, const std::tuple<A...>& a, const std::tuple<B...>& b, F func) {
		tuple_operator(ret, a, b, func, std::make_index_sequence<sizeof...(Ret)>());
	}
}


// Named tuple for C++
// Example code from http://vitiy.info/
// Written by Victor Laskin (victor.laskin@gmail.com)

// Parts of code were taken from: https://gist.github.com/Manu343726/081512c43814d098fe4b
namespace foonathan {
	namespace string_id {
		namespace detail
		{
			using hash_type = uint64_t;

			constexpr hash_type fnv_basis = 14695981039346656037ull;
			constexpr hash_type fnv_prime = 109951162821ull;

			// FNV-1a 64 bit hash
			constexpr hash_type sid_hash(const char *str, hash_type hash = fnv_basis) noexcept
			{
				return *str ? sid_hash(str + 1, (hash ^ *str) * fnv_prime) : hash;
			}
		}
	}
} // foonathan::string_id::detail


namespace fn_detail {

	/// Named parameter (could be empty!)
	template <typename Hash, typename... Ts>
	struct named_param {
		using hash = Hash;                                                  ///< key
		std::tuple<Ts...> value;                                            ///< param's data itself

		named_param(Ts&&... ts) : value(std::forward<Ts>(ts)...) { };        ///< constructor

		template <typename P>
		named_param<Hash, P> operator=(P&& p) { return named_param<Hash, P>(std::forward<P>(p)); };



	};

	template <typename Hash>
	using make_named_param = named_param<Hash>;




	/// Named tuple is just tuple of named params
	template <typename... Params>
	struct named_tuple : public std::tuple<Params...>
	{

		template <typename... Args>
		named_tuple(Args&&... args) : std::tuple<Args...>(std::forward<Args>(args)...) {}

		static const std::size_t error = -1;

		template<std::size_t I = 0, typename Hash>
		constexpr typename std::enable_if<I == sizeof...(Params), const std::size_t>::type
			static get_element_index()
		{
			return error;
		}

		template<std::size_t I = 0, typename Hash>
		constexpr typename std::enable_if<I < sizeof...(Params), const std::size_t>::type
			static get_element_index()
		{
			using elementType = typename std::tuple_element<I, std::tuple<Params...>>::type;
			//return (typeid(typename elementType::hash) == typeid(Hash)) ? I : get_element_index<I + 1, Hash>();
			return (std::is_same<typename elementType::hash, Hash>::value) ? I : get_element_index<I + 1, Hash>();
		}

		template<typename Hash>
		auto& get()
		{
			constexpr std::size_t index = get_element_index<0, Hash>();
			static_assert((index != error), "Wrong named tuple key");
			auto& param = (std::get< index >(static_cast<std::tuple<Params...>&>(*this)));
			return std::get<0>(param.value);
		}

		template<typename NP>
		auto& operator[](NP&& param)
		{
			return get<typename NP::hash>();
		}

	};


}

template <typename... Args>
auto make_named_tuple(Args&&... args)
{
	return fn_detail::named_tuple<Args...>(std::forward<Args>(args)...);
}


template<typename... Ts> struct ArithmeticStruct // a hetoregenous container(tuple) with arithmetic operations
{

	explicit ArithmeticStruct(Ts... args)
	{ // only constructible if all types are arithmetic, ensures that arithmetic operations are posible
		static_assert(util::all_are_arithmetic<Ts...>::value, "ArithmeticStruct can only be constructed if all types are arithmetic!");
		local = std::make_tuple(std::move(args)...);
	}
	std::tuple<Ts...> local; // local storage of data

								// custom operators
	ArithmeticStruct operator+ (ArithmeticStruct& comp) // vec add
	{
		ArithmeticStruct ret(comp);
		util::tuple_operator(ret.local, local, comp.local, [](auto& ret, const auto& a, const auto& b) { ret = a + b; });
		return ret;
	}
	ArithmeticStruct operator- (ArithmeticStruct& comp) // vec add
	{
		ArithmeticStruct ret(comp);
		util::tuple_operator(ret.local, local, comp.local, [](auto& ret, const auto& a, const auto& b) { ret = a - b; });
		return ret;
	}
	ArithmeticStruct operator* (ArithmeticStruct& comp) // vec add
	{
		ArithmeticStruct ret(comp);
		util::tuple_operator(ret.local, local, comp.local, [](auto& ret, const auto& a, const auto& b) { ret = a * b; });
		return ret;
	}

	template<typename T> ArithmeticStruct operator* (T t) // scaling
	{
		static_assert(std::is_arithmetic<T>::value, "A ArithmeticStruct cannot be multiplied with a non artihtmetic object!");
		ArithmeticStruct ret(*this);
		util::for_each_in_tuple(ret.local, t, [](auto& ret, const auto scalar) { ret *= scalar; });
		return ret;
	}

	// calc sum
	double sum()
	{
		double sum = 0; // the data type is an assumption that may fail
		util::for_each_in_tuple(local, sum, [](const auto& element, auto& sum) { sum += element; });
		return sum;
	}
	// calc product
	double product()
	{
		double product = front(); // the data type is an assumption that may fail
		if (front() != 0)
		{
			util::for_each_in_tuple(local, product, [](const auto& element, auto& product) { product *= element; });
			return (product / front()); // divide by 0 evaded
		}
		else
			return 0; // first element was 0, so product will be 0
	}
	bool are_all_positive()
	{
		static_assert((sizeof...(Ts) > 0), "Checking if all of none are positive makes no sense!");
		bool allPositive = true;
		util::for_each_in_tuple(local, allPositive, [](const auto& element, auto& allPositive) { if (element <= 0) allPositive = false; });
		return allPositive;
	}

	template<typename Func> void doForAll(Func func) // execute a lambda function for every tuple element
	{
		util::for_each_in_tuple(local, func); // execution order will likely not be insertion order
	}

	// return first and last tuple element
	inline auto& front()
	{
		static_assert((sizeof...(Ts) > 0), "There is no first tuple element to be accessed!");
		return std::get<0>(local);
	}
	inline auto& back()
	{
		static_assert((sizeof...(Ts) > 0), "There is no last tuple element to be accessed!");
		return std::get<sizeof...(Ts)-1>(local);
		//return std::get<std::tuple_size<decltype(local)>::value - 1>(local); // alternative
	}
};

}

#define param(x) vool::fn_detail::make_named_param< std::integral_constant<vool::foonathan::string_id::detail::hash_type, vool::foonathan::string_id::detail::sid_hash(x)> >{}