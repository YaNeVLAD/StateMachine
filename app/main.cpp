#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "converters/MealyToMoore.hpp"
#include "converters/MooreToMealy.hpp"
#include "printers/MealyToDot.hpp"
#include "printers/MooreToDot.hpp"
#include "readers/MealyFromDot.hpp"
#include "readers/MooreFromDot.hpp"

int main()
{
	const std::string mealy_file = "res/mealy.dot";
	const std::string moore_file = "res/moore.dot";

	try
	{
		{
			std::cout << "Convertion test Mealy->Moore" << std::endl;
			auto mealy = CreateMealyMachineFromDot(mealy_file);
			std::cout << "Input: x1, x2, x2" << std::endl;
			std::cout << mealy.handle_input("x1") << std::endl;
			std::cout << mealy.handle_input("x2") << std::endl;
			std::cout << mealy.handle_input("x2") << std::endl;

			constexpr MealyToMooreConverter converter;
			auto moore = converter(mealy);
			constexpr MooreToMealyConverter converter2;
			auto sameMealy = converter2(moore);
			std::cout << "Input: x1, x2, x2" << std::endl;
			std::cout << sameMealy.handle_input("x1") << std::endl;
			std::cout << sameMealy.handle_input("x2") << std::endl;
			std::cout << sameMealy.handle_input("x2") << std::endl;

			ExportMealyMachineToDot(sameMealy, "same_mealy.dot");
		}

		{
			std::cout << "Convertion test Moore->Mealy" << std::endl;
			auto moore = CreateMooreMachineFromDot(moore_file);
			std::cout << "Input: 1, 2, 2" << std::endl;
			std::cout << moore.handle_input("1") << std::endl;
			std::cout << moore.handle_input("2") << std::endl;
			std::cout << moore.handle_input("2") << std::endl;

			constexpr MooreToMealyConverter converter;
			auto mealy = converter(moore);
			constexpr MealyToMooreConverter converter2;
			auto sameMoore = converter2(mealy);
			std::cout << "Input: 1, 2, 2" << std::endl;
			std::cout << sameMoore.handle_input("1") << std::endl;
			std::cout << sameMoore.handle_input("2") << std::endl;
			std::cout << sameMoore.handle_input("2") << std::endl;

			ExportMooreMachineToDot(sameMoore, "same_moore.dot");
		}
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
