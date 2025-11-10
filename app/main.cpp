#include <fsm.hpp>

#include <regex.hpp>
#include <regular_grammar.hpp>

#include <iostream>

#include "readers/MealyFromDot.hpp"
#include "readers/MooreFromDot.hpp"

int main()
{
	using namespace std::literals;

	const std::string mooreFile2 = "res/moore2.dot";
	const std::string mealyFile2 = "res/mealy2.dot";
	const std::string recognizerFile = "res/recognizer.dot";
	try
	{
		{
			std::cout << "Minimization Test Moore" << std::endl;
			auto moore = CreateMooreMachineFromDot(mooreFile2);
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
			auto mealy = CreateMealyMachineFromDot(mealyFile2);
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

			std::ifstream file("res/recognizer.dot");
			auto recognizer = fsm::dot<fsm::recognizer>(file);

			std::cout << "is_deterministic " << std::boolalpha << recognizer.is_deterministic() << std::endl;
			auto dr = fsm::determinize(recognizer);
			auto mdr = fsm::minimize(dr);

			std::ofstream out1{ "out_recognizer.dot" }, out2{ "out_recognizer2.dot" };
			fsm::dot(out1, dr);
			fsm::dot(out2, mdr);
		}

		{
			std::cout << "Grammar test" << std::endl;

			std::ifstream file("res/grammar.txt");
			auto grammar = fsm::load_grammar(file);

			fsm::save_grammar(std::cout, grammar);

			auto recognizer = fsm::regular_grammar_to_recognizer(grammar);

			std::ofstream out{ "out_grammar_recognizer.dot" };
			fsm::dot(out, recognizer);
		}

		{
			std::cout << "Regex test" << std::endl;

			fsm::base_regex regex("(a*b)*|(b*a)*");

			std::ofstream out{ "out_regex_recognizer.dot" };
			fsm::dot(out, regex.recognizer());
			std::ofstream out2{ "out_regex_recognizer2.dot" };
			fsm::dot(out2, fsm::minimize(fsm::determinize(regex.recognizer())));
		}
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
