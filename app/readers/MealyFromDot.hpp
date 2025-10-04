#ifndef MEALY_FROM_DOT_HPP
#define MEALY_FROM_DOT_HPP

#include <fstream>
#include <regex>
#include <string>

#include "../MealyMachine.hpp"
#include "../StringUtils.hpp"

inline MealyMachine CreateMealyMachineFromDot(const std::string& filename)
{
	using State = std::string;

	MealyState machine;
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Cannot open file: " + filename);
	}

	std::string line;
	std::regex stateRegex(R"lit(^\s*(\w+|"[^"]+")\s*\[label\s*=\s*"([^"]*)"\]\s*;*)lit");
	std::regex transitionRegex(R"lit(^\s*(\w+|"[^"]+")\s*->\s*(\w+|"[^"]+")\s*\[label\s*=\s*"([^"]*)"\]\s*;*)lit");

	std::regex labelRegex("^([^/]+)/(.+)$");

	std::map<std::string, State> stateMap;

	while (std::getline(file, line))
	{

		if (std::smatch match; std::regex_match(line, match, stateRegex))
		{
			std::string name = unquote(match[1]);
			stateMap[name] = State(name);
			machine.stateIds.insert(State(name));
			if (machine.stateIds.size() == 1)
			{
				machine.startStateId = State(name);
				machine.currentStateId = State(name);
			}
		}
		else if (std::regex_match(line, match, transitionRegex))
		{
			std::string srcStateName = unquote(match[1]);
			std::string dstStateName = unquote(match[2]);
			std::string label = unquote(match[3]);

			if (std::smatch labelMatch; std::regex_match(label, labelMatch, labelRegex))
			{
				std::string input = trim(labelMatch[1]);
				std::string output = trim(labelMatch[2]);

				State srcState = stateMap[srcStateName];
				State dstState = stateMap[dstStateName];

				machine.transitions[std::make_pair(srcState, input)] = std::make_pair(dstState, output);
			}
			else
			{
				throw std::runtime_error("Invalid transition label format: " + label);
			}
		}
	}

	file.close();
	return MealyMachine(machine);
}

#endif // MEALY_FROM_DOT_HPP
