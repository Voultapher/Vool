/*
* Vool - Example usage of Vool
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "Tests\AllTests.h"
#include "Tests\ODRTest.h" // include all modules again to check for ODR violation

#include <iostream>
#include <functional>

void runUnitTest(const char* name, std::function<void()> func)
{
	try
	{
		func();
		std::cout << "passed: " << name << "\n";
	}
	catch (std::exception& e)
	{
		std::cout << "failed: " << name << " - " << e.what() << "\n";
	}
}

int main()
{
	std::cout << "\tRunning unit tests:\n\n";

	runUnitTest("Utility", vool::test::test_Utility);
	runUnitTest("Vecmap", vool::test::test_Vecmap);
	runUnitTest("GNP", vool::test::test_GNP);
	runUnitTest("TestSuit", vool::test::test_TestSuit);
	//runUnitTest("TaskQueue", vool::test::test_TaskQueue);

	std::cout << "\n\tAll unit test done!\n\n" << std::flush;

	std::cin.get();

	return 0;
}