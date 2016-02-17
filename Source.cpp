#include <stdio.h>
#include <iostream>
#include <vector>

#include "Tests\VecmapTest.h"
#include "Tests\GNPTest.h"
#include "Tests\TestSuitTest.h"

#include "Vool.h"

// Here some example use cases are shown

int main()
{

	vool::test_Vecmap();
	vool::test_GNP();
	vool::test_TestSuit();

	using Ingredient = vool::ArithmeticStruct<float, int, double, char>;
	auto cookie1 = Ingredient(3.4f, 42, 55.66, 'd');
	auto cookie2 = Ingredient(4.f, 42, 72.3, 'z');
	auto result = cookie1 * cookie2 + cookie1 - cookie2;
	std::cout << "Result sum is: " << result.sum() << " and the first item is: " << result.front() << std::endl;

	std::cout << "\nPrinting all elements of result: \n";
	result.doForAll([](const auto& element) {std::cout << element << std::endl; }); // prints all elements, although may out of order

	//auto emptyTest = vool::ArithmeticStruct<>(); // does not compile
	//emptyTest = emptyTest + emptyTest;
	//std::cout << emptyTest.are_all_positive() << std::endl; // compiler error

	std::vector<int> v;
	//auto wrongTypeTest = vool::ArithmeticStruct<int, float, decltype(v)>(2, 3.f, v); // compiler error

	auto floatMin = std::numeric_limits<float>::min();
	auto floatMax = std::numeric_limits<float>::max();
	auto doubleMin = std::numeric_limits<double>::min();
	auto doubleMax = std::numeric_limits<double>::max();
	auto charMin = std::numeric_limits<char>::min();
	auto charMax = std::numeric_limits<char>::max();
	std::cout << "\nFloat limits: " << floatMin << " " << floatMax << std::endl;
	std::cout << "Double limits: " << doubleMin << " " << doubleMax << std::endl;
	std::cout << "Char limits: " << charMin << " " << charMax << std::endl;

	auto cat = vool::make_named_tuple(param("name") = "Mean", param("age") = 5, param("legs") = 3.3f);
	cat[param("legs")] = 27.67f;
	auto legs = cat[param("legs")];
	printf("\nThe cat named %s has %.3f legs\n", cat[param("name")], legs);

	//cat[param("nam")] = "New"; // compiler error

	int var;
	bool res = std::is_arithmetic<decltype(var)>::value;

	vool::util::needed_arith_type<char, bool, int, float> manquw = 2;
	std::common_type_t<char, bool, int, float> ctwonk = 2;
	vool::util::needed_arith_type<int64_t, uint64_t> impossibleRange = 5; // there is no int type able to hold all the data, so it is double
	std::common_type_t<int64_t, float, uint64_t> impossibleCT = 5;

	char tmp = std::cin.get(); // pause for user input

	return 0;
}