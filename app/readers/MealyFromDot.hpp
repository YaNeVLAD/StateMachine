#ifndef MEALY_FROM_DOT_HPP
#define MEALY_FROM_DOT_HPP

#include <fstream>
#include <regex>
#include <string>

#include "../StringUtils.hpp"
#include <../../state_machine/include/mealy_machine.hpp>

inline fsm::mealy_machine CreateMealyMachineFromDot(const std::string& filename)
{
	mealy_state machine;
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Cannot open file: " + filename);
	}

	std::string line;
	std::regex stateRegex(R"lit(^\s*(\w+|"[^"]+")\s*\[label\s*=\s*"([^"]*)"\]\s*;*)lit");
	std::regex transitionRegex(R"lit(^\s*(\w+|"[^"]+")\s*->\s*(\w+|"[^"]+")\s*\[label\s*=\s*"([^"]*)"\]\s*;*)lit");

	std::regex labelRegex("^([^/]+)/(.+)$");

	std::map<std::string, mealy_state::state_id> stateMap;

	while (std::getline(file, line))
	{

		if (std::smatch match; std::regex_match(line, match, stateRegex))
		{
			std::string name = unquote(match[1]);
			stateMap[name] = name;
			machine.state_ids.insert(name);
			if (machine.state_ids.size() == 1)
			{
				machine.initial_state_id = name;
				machine.current_state_id = name;
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

				auto srcState = stateMap[srcStateName];
				auto dstState = stateMap[dstStateName];

				machine.transitions[std::make_pair(srcState, input)] = std::make_pair(dstState, output);
			}
			else
			{
				throw std::runtime_error("Invalid transition label format: " + label);
			}
		}
	}

	file.close();
	return fsm::mealy_machine(machine);
}

#endif // MEALY_FROM_DOT_HPP
