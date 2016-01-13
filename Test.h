#pragma once

#include "Vool.h"

template<typename A, typename B>
typename std::enable_if<vool::util::all_are_arithmetic<A,B>::value, /*vool::util::needed_arith_type<A,B>*/int >::type add(const A a, const B b)
{
	return a + b;
}