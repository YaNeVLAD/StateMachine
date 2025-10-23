#ifndef MEALY_TO_DOT_HPP
#define MEALY_TO_DOT_HPP

#include <fstream>
#include <stdexcept>

#include <../../state_machine/include/mealy_machine.hpp>

inline void ExportMealyMachineToDot(const fsm::mealy_machine& machine, const std::string& filename)
{
	std::ofstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file for writing: " + filename);
	}

	file << "digraph MealyMachine {\n";
	for (const auto& state_name : machine.state().state_ids)
	{
		file << "    \"" << state_name << "\" [label = \"" << state_name << "\"]\n";
	}
	file << "\n";

	for (const auto& [stateInput, stateOutput] : machine.state().transitions)
	{
		const auto& from_state = stateInput.first;
		const auto& input = stateInput.second;
		const auto& to_state = stateOutput.first;
		const auto& output = stateOutput.second;

		file << "    \"" << from_state << "\" -> \"" << to_state
			 << "\" [label = \"" << input << " / " << output << "\"]\n";
	}

	file << "}\n";
}

#endif // MEALY_TO_DOT_HPP
