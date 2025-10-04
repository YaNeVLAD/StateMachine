#ifndef MOORE_FROM_DOT_HPP
#define MOORE_FROM_DOT_HPP

#include <fstream>
#include <regex>
#include <string>

#include "../StringUtils.hpp"
#include <moore/moore_machine.hpp>

inline fsm::moore_machine CreateMooreMachineFromDot(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Cannot open file: " + filename);
	}

	fsm::moore_machine::state_type state;

	std::regex node_regex(R"lit(^\s*(\w+|"[^"]+")\s*\[label\s*=\s*"[^/]+/\s*([^"]*)"\]\s*;*)lit");
	std::regex edge_regex(R"lit(^\s*(\w+|"[^"]+")\s*->\s*(\w+|"[^"]+")\s*\[label\s*=\s*"([^"]*)"\]\s*;*)lit");

	std::smatch matches;
	std::string line;

	while (std::getline(file, line))
	{
		if (std::regex_match(line, matches, edge_regex))
		{
			if (matches.size() == 4)
			{
				std::string from_state = unquote(matches[1]);
				std::string to_state = unquote(matches[2]);
				std::string input = unquote(matches[3]);

				state.transitions[{ from_state, input }] = to_state;

				state.state_ids.insert(from_state);
				state.state_ids.insert(to_state);
			}
		}
		else if (std::regex_match(line, matches, node_regex))
		{
			if (matches.size() == 3)
			{
				std::string state_id = unquote(matches[1]);
				std::string output = unquote(matches[2]);

				state.outputs[state_id] = output;
				state.state_ids.insert(state_id);

				if (state.initial_state_id.empty())
				{
					state.initial_state_id = state_id;
				}
			}
		}
	}

	if (state.initial_state_id.empty())
	{
		throw std::runtime_error("No states defined in DOT file.");
	}

	state.current_state_id = state.initial_state_id;

	return fsm::moore_machine(state);
}

#endif // MOORE_FROM_DOT_HPP
