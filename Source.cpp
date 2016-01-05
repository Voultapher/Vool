#include <stdio.h>
#include <iostream>
#include <vector>

#include "Vool.h"

// Here some example use cases are shown

int main() {

	using Ingredient = vool::ArithmeticStruct<float, int, double, char>;
	auto cookie1 = Ingredient(3.4f, 42, 55.66, 'd');
	auto cookie2 = Ingredient(4.f, 42, 72.3, 'z');
	auto result = cookie1 * cookie2 + cookie1 - cookie2;
	std::cout << "Result sum is: " << result.sum() << " and the first item is: " << result.front() << std::endl;

	result.doForAll([](const auto& element) {std::cout << element << std::endl; }); // prints all elements, although may out of order

	auto emptyTest = vool::ArithmeticStruct<>();
	emptyTest = emptyTest + emptyTest;
	//std::cout << emptyTest.are_all_positive() << std::endl; // compiler error

	std::vector<int> v;
	//auto wrongTypeTest = vool::ArithmeticStruct<int, float, decltype(v)>(2, 3.f, v); // compiler error

	auto cat = vool::make_named_tuple(param("name") = "Mean", param("age") = 5, param("legs") = 3.3f);
	cat[param("legs")] = 27.67f;
	auto legs = cat[param("legs")];
	printf("The cat named %s has %.3f legs\n", cat[param("name")], legs);

	//cat[param("nam")] = "New"; // compiler error

	char tmp = std::cin.get(); // pause for user input

	return 0;
}