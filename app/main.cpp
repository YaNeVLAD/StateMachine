#include <mealy/minimization.hpp>
#include <moore/minimization.hpp>

#include <iostream>

#include "printers/MealyToDot.hpp"
#include "printers/MooreToDot.hpp"
#include "readers/MealyFromDot.hpp"
#include "readers/MooreFromDot.hpp"

int main()
{
	using namespace std::literals;

	const std::string moore_file2 = "res/moore2.dot";
	const std::string mealy_file2 = "res/mealy2.dot";

	try
	{
		{
			std::cout << "Minimization Test Moore" << std::endl;
			auto moore = CreateMooreMachineFromDot(moore_file2);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z1") << std::endl;
			std::cout << moore.handle_input("z2") << std::endl;

			auto minMoore = fsm::minimize(moore);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z1") << std::endl;
			std::cout << minMoore.handle_input("z2") << std::endl;

			ExportMooreMachineToDot(minMoore, "min_moore2.dot");
		}

		{
			std::cout << "Minimization Test Mealy" << std::endl;
			auto mealy = CreateMealyMachineFromDot(mealy_file2);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z1") << std::endl;
			std::cout << mealy.handle_input("z2") << std::endl;

			auto minMealy = fsm::minimize(mealy);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z1") << std::endl;
			std::cout << minMealy.handle_input("z2") << std::endl;

			ExportMealyMachineToDot(minMealy, "min_mealy2.dot");
		}
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
