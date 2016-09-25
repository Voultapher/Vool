/*
* Vool - Generic metaprogramming library focused around variadic templates
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
*/

#ifndef VOOL_UTILITY_H_INCLUDED
#define VOOL_UTILITY_H_INCLUDED

#include <vector>
#include <tuple>

#define NOT !

namespace vool
{

	namespace util
	{
		// variadic compile time property check
		template < bool CONDITION > struct true_if : std::conditional< CONDITION, std::true_type, std::false_type >::type {};

		template<typename T> struct is_double : std::is_same<T, double> {};
		template<typename A> struct is_vector { static const bool value = false; };
		template<typename T, typename A> struct is_vector<std::vector<T, A>> { static const bool value = true; };

		template<bool...> struct bool_pack;
		template<bool... b> using all_true = std::is_same<bool_pack<true, b...>, bool_pack<b..., true>>;
		template<bool... b> using all_false = std::is_same<bool_pack<false, b...>, bool_pack<b..., false>>;
		template<bool... b> using some_true = true_if<NOT all_true<b...>::value && NOT all_false<b...>::value>;

		template<typename... Ts> struct all_are_arithmetic : true_if<all_true<std::is_arithmetic<Ts>::value...>::value> {};
		template<typename... Ts> struct none_are_double : true_if<all_false<is_double<Ts>::value...>::value> {};
		template<typename... Ts> struct none_are_floating_point : true_if<all_false<std::is_floating_point<Ts>::value...>::value> {};
		template<typename TO, typename... FROM> struct all_are_convertible : true_if<all_true<std::is_convertible<TO, FROM>::value...>::value> {};
		template<typename... Ts> struct all_are_vector : true_if<all_true<is_vector<Ts>::value...>::value> {};

		template<typename INT_T, typename UNIT_T, typename... Ts> struct is_impossible_int :
			true_if<
			(some_true<std::is_same<Ts, INT_T>::value...>::value
				&& some_true<std::is_same<Ts, UNIT_T>::value...>::value)
			|| (some_true<sizeof(INT_T) <= sizeof(Ts) ...>::value
				&& none_are_floating_point<Ts...>::value)
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
				std::conditional_t<is_impossible_int<int64_t, uint64_t, Ts...>::value, double,
				std::conditional_t<is_impossible_int<int32_t, uint32_t, Ts...>::value, int64_t,
				std::conditional_t<is_impossible_int<int16_t, uint16_t, Ts...>::value, int32_t,
				std::conditional_t<is_impossible_int<int8_t, uint8_t, Ts...>::value, int16_t,
				std::common_type_t<Ts...>>>>>;
		};

		template<typename... Ts> using needed_arith_type_t = typename common_type_helper<
			(sizeof...(Ts) > 0), (all_are_arithmetic<Ts...>::value), Ts...>::type;

		template<typename T = void> struct nope
		{
			static_assert(NOT std::is_same<T, void>::value, "You may nope!");
		};


		// tuple iteration
		template<typename... Ts> void wrapper(Ts&&... args) { }

		//  iteration, only tuple as argument
		template<class F, class... Ts, std::size_t... Is>
		void for_each_in_tuple(std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>)
		{
			wrapper((func(std::get<Is>(tuple)), 0)...);
		}

		template<class F, class...Ts>
		void for_each_in_tuple(std::tuple<Ts...>& tuple, F func)
		{
			for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
		}

		// iteration, with tuple + scalar
		template<typename... Ts, typename S, typename F, std::size_t... Is>
		void for_each_in_tuple(
			std::tuple<Ts...>& tuple,
			S& scalar,
			F func,
			std::index_sequence<Is...>
		)
		{
			wrapper((func(std::get<Is>(tuple), scalar), 0)...);
		}

		template<typename... Ts, typename S, typename F>
		void for_each_in_tuple(std::tuple<Ts...>& tuple, S& scalar, F func)
		{
			for_each_in_tuple(tuple, scalar, func, std::make_index_sequence<sizeof...(Ts)>());
		}

		// iteration, with 3 tuples
		template<typename... Ret, typename... A, typename... B, class F, std::size_t... Is>
		void tuple_operator(
			std::tuple<Ret...>& ret,
			const std::tuple<A...>& a,
			const std::tuple<B...>& b,
			F func,
			std::index_sequence<Is...>)
		{
			wrapper((func(std::get<Is>(ret), std::get<Is>(a), std::get<Is>(b)), 0)...);
		}

		template<typename... Ret, typename... A, typename... B, class F>
		void tuple_operator(
			std::tuple<Ret...>& ret,
			const std::tuple<A...>& a,
			const std::tuple<B...>& b,
			F func)
		{
			tuple_operator(ret, a, b, func, std::make_index_sequence<sizeof...(Ret)>());
		}
	}

	// a hetoregenous container(tuple) with arithmetic operations
	template<typename... Ts> class ArithmeticStruct
	{
	private:
		std::tuple<Ts...> _local;

	public:
		using arith_t = util::needed_arith_type_t<Ts...>;

		explicit ArithmeticStruct(Ts... args)
		{
			// only constructible if all types are arithmetic
			// ensures that arithmetic operations are posible
			static_assert(util::all_are_arithmetic<Ts...>::value,
				"ArithmeticStruct can only be constructed if all types are arithmetic!");
			_local = std::make_tuple(std::move(args)...);
		}

		// custom operators
		ArithmeticStruct operator+ (ArithmeticStruct& comp) // vec add
		{
			ArithmeticStruct ret(comp);
			util::tuple_operator(ret._local, _local, comp._local,
				[](auto& ret, const auto& a, const auto& b) { ret = a + b; });
			return std::move(ret);
		}
		ArithmeticStruct operator- (ArithmeticStruct& comp) // vec add
		{
			ArithmeticStruct ret(comp);
			util::tuple_operator(ret._local, _local, comp._local,
				[](auto& ret, const auto& a, const auto& b) { ret = a - b; });
			return std::move(ret);
		}
		ArithmeticStruct operator* (ArithmeticStruct& comp) // vec add
		{
			ArithmeticStruct ret(comp);
			util::tuple_operator(ret._local, _local, comp._local,
				[](auto& ret, const auto& a, const auto& b) { ret = a * b; });
			return std::move(ret);
		}

		template<typename T> ArithmeticStruct operator* (T t) // scaling
		{
			static_assert(std::is_arithmetic<T>::value,
				"An ArithmeticStruct cannot be multiplied with a non artihtmetic object!");

			ArithmeticStruct ret(*this);
			util::for_each_in_tuple(ret._local, t,
				[](auto& ret, const auto scalar) { ret *= scalar; });
			return std::move(ret);
		}

		// calc sum
		arith_t sum()
		{
			arith_t sum = {};
			util::for_each_in_tuple(_local, sum,
				[](const auto& element, auto& sum) { sum += element; });
			return sum;
		}

		// calc product
		arith_t product()
		{
			arith_t product = front();
			if (front() != 0)
			{
				util::for_each_in_tuple(_local, product,
					[](const auto& element, auto& product) { product *= element; });
				return (product / front()); // divide by 0 evaded
			}
			else
				return{}; // first element was 0, so product will be 0
		}

		bool are_all_positive()
		{
			static_assert((sizeof...(Ts) > 0),
				"Checking if all of none are positive makes no sense!");
			bool allPositive = true;

			util::for_each_in_tuple(
				_local, allPositive,
				[](const auto& element, auto& allPositive)
			{
				if (element <= 0) allPositive = false;
			}
			);
			return allPositive;
		}

		// execute a lambda function for every tuple element
		template<typename Func> void doForAll(Func func)
		{
			// execution order will likely not be insertion order
			util::for_each_in_tuple(_local, func);
		}

		// return first and last tuple element
		template<typename...> inline auto front()
		{
			static_assert((sizeof...(Ts) > 0), "There is no first tuple element to be accessed!");
			return std::get<0>(_local);
		}
		template<typename...> inline auto& back()
		{
			static_assert((sizeof...(Ts) > 0), "There is no last tuple element to be accessed!");
			return std::get<sizeof...(Ts)-1>(_local);
			//return std::get<std::tuple_size<decltype(_local)>::value - 1>(_local); // alternative
		}
	};

}

#undef NOT

#endif // VOOL_UTILITY_H_INCLUDED