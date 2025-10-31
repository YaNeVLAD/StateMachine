#ifndef MOORE_TO_DOT_HPP
#define MOORE_TO_DOT_HPP

#include <fstream>

#include <moore_machine.hpp>

inline void ExportMooreMachineToDot(const fsm::moore_machine& machine, const std::string& filename)
{
	std::ofstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file for writing: " + filename);
	}

	file << "digraph MooreMachine {\n";
	for (const auto& state_name : machine.state().state_ids)
	{
		auto it = machine.state().outputs.find(state_name);
		if (it == machine.state().outputs.end())
		{
			throw std::runtime_error("Inconsistent Moore machine: No output for state " + state_name);
		}
		const auto& output = it->second;

		file << "    \"" << state_name << "\" [label = \"" << state_name << " / " << output << "\"]\n";
	}
	file << "\n";

	for (const auto& transition : machine.state().transitions)
	{
		const auto& from_state = transition.first.first;
		const auto& input = transition.first.second;
		const auto& to_state = transition.second;

		file << "    \"" << from_state << "\" -> \"" << to_state
			 << "\" [label = \"" << input << "\"]\n";
	}

	file << "}\n";
}

#endif // MOORE_TO_DOT_HPP
