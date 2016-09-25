/*
* Vool - Example usage of Vool
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
*/

#include "Tests\AllTests.h"
#include "Tests\ODRTest.h"

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
	runUnitTest("TaskQueue", vool::test::test_TaskQueue);

	std::cout << "\n\tAll unit test done!\n\n" << std::flush;

	std::cin.get();

	return 0;
}