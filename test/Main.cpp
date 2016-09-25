/*
* Vool - run unit tests
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
*/

#include "AllTests.h"
#include "ODRTest.h"

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

	runUnitTest("Utility", vool::tests::test_Utility);
	runUnitTest("Vecmap", vool::tests::test_Vecmap);
	runUnitTest("GNP", vool::tests::test_GNP);
	runUnitTest("TestSuit", vool::tests::test_TestSuit);
	runUnitTest("TaskQueue", vool::tests::test_TaskQueue);

	std::cout << "\n\tAll unit test done!\n\n" << std::flush;

	std::cin.get();

	return 0;
}