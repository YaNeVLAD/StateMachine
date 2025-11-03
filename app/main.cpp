#include <fsm.hpp>

#include <iostream>

#include "readers/MealyFromDot.hpp"
#include "readers/MooreFromDot.hpp"

int main()
{
	using namespace std::literals;

	const std::string moore_file2 = "res/moore2.dot";
	const std::string mealy_file2 = "res/mealy2.dot";
	const std::string recognizer_file = "res/recognizer.dot";

	try
	{
		{
			std::cout << "Minimization Test Moore" << std::endl;
			auto moore = CreateMooreMachineFromDot(moore_file2);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl
					  << moore.handle_input("z1") << std::endl
					  << moore.handle_input("z2") << std::endl
					  << moore.handle_input("z2") << std::endl
					  << moore.handle_input("z1") << std::endl
					  << moore.handle_input("z2") << std::endl
					  << moore.handle_input("z1") << std::endl
					  << moore.handle_input("z1") << std::endl
					  << moore.handle_input("z2") << std::endl;

			auto minMoore = fsm::minimize(moore);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl
					  << minMoore.handle_input("z1") << std::endl
					  << minMoore.handle_input("z2") << std::endl
					  << minMoore.handle_input("z2") << std::endl
					  << minMoore.handle_input("z1") << std::endl
					  << minMoore.handle_input("z2") << std::endl
					  << minMoore.handle_input("z1") << std::endl
					  << minMoore.handle_input("z1") << std::endl
					  << minMoore.handle_input("z2") << std::endl;

			std::ofstream out("min_moore2.dot");
			fsm::dot(out, minMoore);
		}

		{
			std::cout << "Minimization Test Mealy" << std::endl;
			auto mealy = CreateMealyMachineFromDot(mealy_file2);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl
					  << mealy.handle_input("z1") << std::endl
					  << mealy.handle_input("z2") << std::endl
					  << mealy.handle_input("z2") << std::endl
					  << mealy.handle_input("z1") << std::endl
					  << mealy.handle_input("z2") << std::endl
					  << mealy.handle_input("z1") << std::endl
					  << mealy.handle_input("z1") << std::endl
					  << mealy.handle_input("z2") << std::endl;

			auto minMealy = fsm::minimize(mealy);
			std::cout << "Input: z1, z2, z2, z1, z2, z1, z1, z2" << std::endl
					  << minMealy.handle_input("z1") << std::endl
					  << minMealy.handle_input("z2") << std::endl
					  << minMealy.handle_input("z2") << std::endl
					  << minMealy.handle_input("z1") << std::endl
					  << minMealy.handle_input("z2") << std::endl
					  << minMealy.handle_input("z1") << std::endl
					  << minMealy.handle_input("z1") << std::endl
					  << minMealy.handle_input("z2") << std::endl;

			std::ofstream out("min_mealy2.dot");
			fsm::dot(out, minMealy);
		}

		{
			std::cout << "Recognizer test" << std::endl;
			auto recognizer = fsm::recognizer::from_dot(recognizer_file);
			std::cout << "is_deterministic " << std::boolalpha << recognizer.is_deterministic() << std::endl;
			auto dr = fsm::determinize(recognizer);
			auto mdr = fsm::minimize(dr);

			std::ofstream out1{ "out_recognizer.dot" }, out2{ "out_recognizer2.dot" };
			fsm::dot(out1, dr);
			fsm::dot(out2, mdr);
		}
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
