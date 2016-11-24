/*
* Vool - utility
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#ifndef VOOL_UTILITY_H_INCLUDED
#define VOOL_UTILITY_H_INCLUDED

#include <vector>
#include <string>
#include <tuple>

namespace vool
{

namespace util
{

// --- variadic compile time property check ---

template <bool Cond> struct true_if_t : std::conditional<Cond,
	std::true_type,
	std::false_type
>::type {};

template<class B> struct negation : std::integral_constant<bool, !B::value> {};

template<bool Cond, typename A, typename B> using conditional_t =
	typename std::conditional<Cond, A, B>::type;


// --- variadic logical binary metafunctions ---

template<bool...> struct bool_pack;
template<bool... b> using conjunction = std::is_same<
	bool_pack<true, b...>,
	bool_pack<b..., true>
>;
template<bool... b> using all_false = std::is_same<
	bool_pack<false, b...>,
	bool_pack<b..., false>
>;
template<bool... b> using disjunction = true_if_t<
	negation<conjunction<b...>>::value &&
	negation<all_false<b...>>::value
>;

template<typename... Ts> struct all_are_arithmetic : true_if_t<
	conjunction<std::is_arithmetic<Ts>::value...>::value
> {};

// --- common airthmetic type ---

template<typename INT_T, typename UNIT_T, typename... Ts>struct is_int_overflow :
	true_if_t<
	(disjunction<std::is_same<Ts, INT_T>::value...>::value
		&& disjunction<std::is_same<Ts, UNIT_T>::value...>::value)
	|| (disjunction<sizeof(INT_T) <= sizeof(Ts) ...>::value
		&& all_false<std::is_floating_point<Ts>::value...>::value)
	> {};

template<bool NOT_empty, bool arith, typename... Ts> struct common_type_helper
{
	static_assert(NOT_empty, "Using need_arith_type you need at least one type!");
	static_assert(arith, "Using need_arith_type, all types have to be arithmetic!");
	using type = void;
};

template<typename... Ts> struct common_type_helper<true, true, Ts...>
{
	using type =
		conditional_t<is_int_overflow<int64_t, uint64_t, Ts...>::value, double,
		conditional_t<is_int_overflow<int32_t, uint32_t, Ts...>::value, int64_t,
		conditional_t<is_int_overflow<int16_t, uint16_t, Ts...>::value, int32_t,
		conditional_t<is_int_overflow<int8_t, uint8_t, Ts...>::value, int16_t,
		typename std::common_type<Ts...>::type>>>>;
};

template<typename... Ts>
using common_arithmetic_t = typename common_type_helper<
	(sizeof...(Ts) > 0),
	(all_are_arithmetic<Ts...>::value),
	Ts...
>::type;


// --- fold ---

namespace detail
{
template<typename F, typename Tuple, std::size_t... Is>
void tuple_fold(F&& func, Tuple&& tuple, std::index_sequence<Is...>)
{
	static_cast<void>(
		std::initializer_list<int>
	{
		(std::forward<F>(func)(
			std::get<Is>(std::forward<Tuple>(tuple))),
			0
			)...
	}
	);
}
}

template<typename F, typename Tuple> void fold(
	F&& func,
	Tuple&& tuple
)
{
	detail::tuple_fold(
		std::forward<F>(func),
		std::forward<Tuple>(tuple),
		std::make_index_sequence<
			std::tuple_size<typename std::decay<Tuple>::type>::value
		>{}
	);
}

template<typename F, typename...Ts> void fold(
	F&& func,
	Ts&&... args
)
{
	static_cast<void>(
		std::initializer_list<int>
		{ (std::forward<F>(func)(std::forward<Ts>(args)), 0)... }
	);
}

// ----- Concepts -----

struct nonesuch;

namespace detail
{
//template <typename T, typename ...>
//struct alias { using type = T; };
}

template<
	typename Default,
	template<typename...> class Expression,
	typename... Ts
>
class detected_or
{
private:
	template<
		template<typename...> class Alias,
		typename... Args,
		typename Ret_t = Alias<Args...>
	>
	static auto ov(int ph)->Ret_t;

	template<
		template<typename...> class Alias,
		typename... Args
	>
	static Default ov(...);

public:
	using type = decltype(ov<Expression, Ts...>(0));

	using value_t = std::negation<std::is_same<
		type,
		Default
	>>;

	static constexpr bool value = value_t::value;
};
template<
	typename Default,
	template <typename...> class Expression,
	typename... Ts
>
using detected_or_t = typename detected_or<Default, Expression, Ts...>::type;


template<template <typename...> class Expression, typename... Ts>
using is_detected = typename detected_or<
	vool::util::nonesuch,
	Expression,
	Ts...
>;
template<template <typename...> class Expression, typename... Ts>
constexpr bool is_detected_v = is_detected<Expression, Ts...>::value;

template<template <typename...> class Expression, typename... Ts>
using is_detected_t = typename is_detected<Expression, Ts...>::type;


template<
	typename Expected,
	template <typename...> class Expression,
	typename... Ts
>
using is_detected_exact = std::is_same<
	typename std::decay<Expected>::type,
	typename std::decay<typename is_detected<Expression, Ts...>::type>::type
>;
template<
	typename Expected,
	template <typename...> class Expression,
	typename... Ts
>
constexpr bool is_detected_exact_v = is_detected_exact<
	Expected,
	Expression,
	Ts...
>::value;


template<
	template <typename...> class Expected,
	template <typename...> class Expression,
	typename... Ts
>
using is_detected_expression = is_detected_exact<
	typename detected_or<struct is_expression_tag, Expected, Ts...>::type,
	Expression,
	Ts...
>;
template<
	template <typename...> class Expected,
	template <typename...> class Expression,
	typename... Ts
>
constexpr bool is_detected_expression_v = is_detected_expression<
	Expected,
	Expression,
	Ts...
>::value;


// --- def ---
template<typename T> using def_begin = decltype(std::declval<T>().begin());
template<typename T> using def_end = decltype(std::declval<T>().end());

template<typename T> using def_iterator_deref = decltype(
	std::declval<typename T::iterator>().operator*()
);

/*template<typename T> auto make_hash(T&& val)
{
	return std::hash<T>{}(std::forward<T>(val));
}

template<typename T> using def_hash = decltype(
	make_hash(std::declval<T>())
);*/

template<typename T>
using def_to_string = decltype(std::to_string(std::declval<T>()));

// --- type ---
template<typename T> using type_iterator = typename T::iterator;
template<typename T> using type_iterator_const = const typename T::iterator;

template<typename T> using type_val = typename T::value_type;
template<typename T> using type_val_const = const typename T::value_type;

template<typename T> using type_class = typename std::is_class<T>::type;

// --- has ---
template<typename T> using has_range_begin = true_if_t<
	is_detected_expression<type_iterator, def_begin, T>::value
	|| is_detected_expression<type_iterator_const, def_begin, T>::value
>;
template<typename T> using has_range_end = true_if_t<
	is_detected_expression<type_iterator, def_end, T>::value
	|| is_detected_expression<type_iterator_const, def_end, T>::value
>;

template<typename T> using has_range_iterator = true_if_t<
	is_detected_expression<type_val, def_iterator_deref, T>::value
	|| is_detected_expression<type_val_const, def_iterator_deref, T>::value
>;

// --- is ---
template<typename T> using is_range = true_if_t<
	has_range_begin<T>::value
	&& has_range_end<T>::value
	&& has_range_iterator<T>::value
>;
template<typename T> constexpr bool is_range_v = is_range<T>::value;

template<typename T> using is_printable = true_if_t<
	is_detected<def_to_string, T>::value
	|| std::is_convertible<std::string, T>::value
>;
template<typename T> constexpr bool is_printable_v = is_printable<T>::value;

template<typename T> using is_hashable = true_if_t<
	//is_detected<def_hash, T>::value
	true
>;
template<typename T> constexpr bool is_hashable_v = is_hashable<T>::value;

template<typename T> using is_vector = std::is_same<
	std::vector<typename is_detected_t<type_val, T>>,
	T
>;
template<typename T> constexpr bool is_vector_v = is_vector<T>::value;


// --- requires ---
template<
	typename Result,
	typename T,
	template<typename> class... Concepts
>
using requires = std::enable_if_t<
	conjunction<Concepts<T>::value...>::value,
	Result
>;

template<
	typename Result,
	typename T,
	template<typename> class... Concepts
>
using fallback = std::enable_if_t<
	all_false<Concepts<T>::value...>::value,
	Result
>;

}
}

#endif // VOOL_UTILITY_H_INCLUDED